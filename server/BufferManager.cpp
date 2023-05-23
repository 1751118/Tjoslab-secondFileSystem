#include "BufferManager.h"
#include "Kernel.h"
#include <string.h>
#include <stdio.h>

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

/**
 * 初始化缓存，包括控制块和缓存的链接，排成自由缓存队列及缓存互斥锁的初始化
*/
void BufferManager::Initialize()
{
	printf("Initializing BufferManager......\n");
	int i;
	Buf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	// this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	for(i = 0; i < NBUF; i++)
	{
		// 控制的
		bp = &(this->m_Buf[i]);
		// bp->b_dev = -1;
		// 存的
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		// 链表中插入
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		bp->b_flags = Buf::B_BUSY;
		pthread_mutex_init(&bp->b_lock, NULL);
		pthread_mutex_lock(&bp->b_lock);
		
		Brelse(bp);

	}
	// this->m_DeviceManager = &Kernel::Instance().GetDeviceManager();
	return;
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp;
	Buf* head = &this->GetBFreeList();

	for(bp = head->b_forw; bp != head; bp = bp->b_forw)
	{
		if(bp->b_blkno != blkno) continue;

		//找到一块属于blkno的缓存，拿过来直接用，上锁返回
		bp->b_flags |= Buf::B_BUSY;
		pthread_mutex_lock(&bp->b_lock);
		return bp;
	}

	//如果没有属于blkno的缓存，看看有没有其他没有上锁的缓存
	bool has = false;
	for(bp = head->b_forw; bp != head; bp = bp->b_forw)
	{
		if(pthread_mutex_trylock(&bp->b_lock) == 0){
			has = true;
			break;
		}
	}
	if(!has){
		//如果没有的话，需要等待，这里设定等待队头的的缓存块
		bp = head->b_forw;
		pthread_mutex_lock(&bp->b_lock);
	}

	//这里还要判断是不是有延迟写标识，有的话需要先写入
	if(bp->b_flags & Buf::B_DELWRI){
		this->Bwrite(bp);
		pthread_mutex_lock(&bp->b_lock); 
	}
	bp->b_blkno = blkno;
	bp->b_flags = Buf::B_BUSY;

	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息 
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	pthread_mutex_unlock(&bp->b_lock);
	return;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	cout << "来拿缓存块" << blkno << endl;
	bp = this->GetBlk(blkno);
	cout << "缓存块" << blkno << "拿到了" <<endl;
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	
	if(bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;
	
	memcpy(bp->b_addr, &this->start_addr[BufferManager::BUFFER_SIZE * bp->b_blkno], BufferManager::BUFFER_SIZE);

	/* 
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */
	// this->m_DeviceManager->GetBlockDevice(Utility::GetMajor(dev)).Strategy(bp);
	// /* 同步读，等待I/O操作结束 */
	// this->IOWait(bp);
	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	memcpy(&this->start_addr[BufferManager::BUFFER_SIZE * bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
	this->Brelse(bp);

	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for(unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush()
{
	Buf* bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */
// loop:
// 	X86Assembly::CLI();
	for(bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if( (bp->b_flags & Buf::B_DELWRI) /*&& (dev == DeviceManager::NODEV || dev == bp->b_dev) */)
		{
			//将这块缓存从队列中取出
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			//插入到队头，供下次取用
			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;

			//将其写入文件
			this->Bwrite(bp);			
		}
	}
	
	return;
}

void BufferManager::GetError(Buf* bp)
{
	User& u = Kernel::Instance().GetUser();

	if (bp->b_flags & Buf::B_ERROR)
	{
		u.u_error = EIO;
	}
	return;
}

Buf* BufferManager::InCore(int blkno)
{
	Buf* bp;
	// Devtab* dp;
	// short major = Utility::GetMajor(adev);

	//dp = this->m_DeviceManager->GetBlockDevice(major).d_tab;
	Buf* dp = &this->bFreeList;
	for(bp = dp->b_forw; bp != (Buf *)dp; bp = bp->b_forw)
	{
		if(bp->b_blkno == blkno)
			return bp;
	}
	return NULL;
}


Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

