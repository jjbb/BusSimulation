#ifndef _MFI_POOL_HEADER_
#define _MFI_POOL_HEADER_

#include "mfi_list.h"
#include "mfiapi.h"

#define DEF_MEM_PAGE    128
#define MEM_F_SHARED    0x1                /* 标示对应的池允许共用 */

/* 每个池的相关信息 */
typedef struct pool_head {
	void                     **free_list;     //空闲内存块链表，每个内存块的前四个字节存储下个存储块的地址
  MfiUInt32                used;            /* how many chunks are currently in use 分配出去正在使用的块数量*/
  MfiUInt32                allocated;        /* how many chunks have been allocated 已分配的块总数，包括正在使用和留在池中空闲两部分*/
  MfiUInt32                limit;            /* hard limit on the number of chunks 最大分配数量限制,0为无限*/
  MfiUInt32                minavail;        /* how many chunks are expected to be used 释放不必要块时，保留的合适的最小数量*/
  MfiUInt32                size;            /* chunk size */
  MfiUInt32                flags;            /* MEM_F_* */
//  MfiUInt32                users;            /* number of pools sharing this zone 记录正在使用某一尺寸池的用户数量*/
  pthread_mutex_t       lock;
  struct list_head         list;            //使用linux自带的list，组织所有大小类型的pool
}pool_head,*pool_head_p;

typedef struct mem_pools{
  MfiUInt32                limit;            /* hard limit on the number of chunks */
  MfiUInt32                minavail;        /* how many chunks are expected to be used */
  MfiUInt32                mempage;
	struct list_head         pools;
	pthread_rwlock_t         rwlock;
}mem_pools,*mem_pools_p;

/* 池创建 */
/* Try to find an existing shared pool with the same characteristics and
 * returns it, otherwise creates this one. NULL is returned if no memory
 * is available for a new creation.
 */

MfiStatus pool_create(mem_pools_p mpools, MfiUInt32 size, pool_head_p* pool_head_r);

/* 池销毁 */
/*
 * This function destroys a pool by freeing it completely, unless it's still
 * in use. This should be called only under extreme circumstances. It always
 * returns NULL if the resulting pool is empty, easing the clearing of the old
 * pointer, otherwise it returns the pool.
 */
extern void* pool_destroy(mem_pools_p mpools, pool_head_p pool);

/* 把池中的空闲的元素都给释放掉 */
/*
 * This function frees whatever can be freed in pool <pool>.
 */
extern void pool_clear(pool_head_p pool);

/* 把池中非必要的元素给释放掉 */
/*
 * This function frees whatever can be freed in all pools, but respecting
 * the minimum thresholds imposed by owners. It takes care of avoiding
 * recursion because it may be called from a signal handler.
 */
extern void pool_flush_nonessential(mem_pools_p mpools);

/* 动态分配一个 pool 元素大小的内存空间 */
/* Allocate a new entry for pool <pool>, and return it for immediate use.
 * NULL is returned if no memory is available for a new creation.
 */
extern void* pool_alloc(mem_pools_p mpools, pool_head_p pool);

/*
 * Returns a pointer to type <type> taken from the
 * pool <pool_type> or dynamically allocated. In the
 * first case, <pool_type> is updated to point to the
 * next element in the list.
 */
//从池中获取块

/*#define pool_alloc(pool) \
({ \
  void *__p; \
  if((__p = (pool)->free_list) == NULL)            \
    __p = pool_refill_alloc(pool); \
  else { \
    (pool)->free_list = *(void **)(pool)->free_list; \
  	(pool)->used++;  \
  } \
  __p; \
})*/

/*
 * Puts a memory area back to the corresponding pool.
 * Items are chained directly through a pointer that
 * is written in the beginning of the memory area, so
 * there's no need for any carrier cell. This implies
 * that each memory area is at least as big as one
 * pointer. Just like with the libc's free(), nothing

 * is done if <ptr> is NULL.

 */
//将一个块回收到池中
int pool_free(pool_head_p pool, void *mem_chunk);
/*#define pool_free(pool, ptr) \
{ \
  if((ptr) != NULL) { \
  	*(void **)(ptr) = (void *)(pool)->free_list;    \
		(pool)->free_list = (void *)(ptr);    \
		(pool)->used--;                \
  } \
)
*/

#endif
