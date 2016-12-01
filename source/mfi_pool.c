#include "mfi_pool.h"
#include "pthread.h"
#include <stdlib.h>

/* 初始化一个内存池 */
//struct list_head是linux提供的链表结构，只包含指向该结构的next和prew两个指针成员
//LIST_HEAD_INIT是Linux提供的一个宏，将list_head结构初始化为一个next、prew指向自身的链表

MfiStatus MemPools_Init(mem_pools_p mpools, MfiUInt32 limit, MfiUInt32 minavail, MfiUInt32 mempage)
{
	if(pthread_rwlock_init(&(mpools->rwlock),MFI_NULL)!=0)
		return MFI_ERROR_SYSTEM_ERROR;
		
	INIT_LIST_HEAD(&(mpools->pools));
	mpools->limit=limit;
	mpools->minavail=minavail;
	mpools->mempage=mempage;
	return MFI_SUCCESS;
}

/* 为一个内存池创建新尺寸的池 */
/* Try to find an existing shared pool with the same characteristics and
 * returns it, otherwise creates this one. NULL is returned if no memory
 * is available for a new creation.
 */
MfiStatus pool_create(mem_pools_p mpools, MfiUInt32 size, pool_head_p* pool_head_r)
{
  struct pool_head *pool;
  struct pool_head *entry;
  struct list_head *start;
	MfiUInt32	        align;
	
  /* We need to store at least a (void *) in the chunks. Since we know
   * that the malloc() function will never return such a small size,
   * let's round the size up to something slightly bigger, in order to
   * ease merging of entries. Note that the rounding is a power of two.
   */
	//该方法的计算结果是：不满page的size为page.即按照page的倍数分配空间
	align=mpools->mempage;
  size = (size + align - 1) & -align;

  start = &(mpools->pools);
  pool = NULL;
  
  pthread_rwlock_rdlock(&(mpools->rwlock));
	//list_for_each_entry是一个宏，可以循环返回包含list_head的链表每一个节点的指针到entry
  list_for_each_entry(entry, &(mpools->pools), list) {
    if (entry->size == size) {
      /* either we can share this place and we take it, or
       * we look for a sharable one or for the next position
       * before which we will insert a new one.
       */
      pthread_mutex_lock(&(entry->lock));
      if(entry->flags==1){
       	pool = entry;
       	pthread_mutex_unlock(&(entry->lock));
       	break;
      }
      pthread_mutex_unlock(&(entry->lock));
    }
    else if (entry->size > size) {
      /* insert before this one */
      start = &entry->list;
      break;
    }
  }
	pthread_rwlock_unlock(&(mpools->rwlock));
	
  if (!pool) {
		//calloc能分配n*size的空间，并初始化
    pool = calloc(1, sizeof(*pool));
    if (!pool)
      return MFI_ERROR_ALLOC;

    pool->size = size;
    pool->limit=mpools->limit;
    pool->minavail=mpools->minavail;
    pool->flags=1;

		pthread_rwlock_wrlock(&(mpools->rwlock));
		//添加某种大小类型的池
    list_add_tail(&pool->list,start);
    pthread_rwlock_unlock(&(mpools->rwlock));
  }

/*  
  pthread_mutex_lock(&(pool->lock));
  pool->users++;
  pthread_mutex_unlock(&(pool->lock));
  */
  
  *pool_head_r=pool;
  
  return MFI_SUCCESS;
}

/* 池销毁 */
void* pool_destroy(mem_pools_p mpools, pool_head_p pool)
{
  if(pool)
  {
    pool_clear(pool);            // 请看池中的空闲的元素
    
    pthread_mutex_lock(&(pool->lock));
    if(pool->used){
    	pthread_mutex_unlock(&(pool->lock));
      return pool;
    }
		pool->flags=0;
		pthread_mutex_unlock(&(pool->lock));
    pthread_rwlock_wrlock(&(mpools->rwlock));
    list_del(&pool->list);    // 从 pools 链表中删除
    pthread_rwlock_unlock(&(mpools->rwlock));
    free(pool);                // 把 pool 结构体占用的内存给释放了
    
 /*   pool->users--;
    if (!pool->users)
    {
      list_del(&pool->list);    // 从 pools 链表中删除
      free(pool);                // 把 pool 结构体占用的内存给释放了
    }
    */
  }
  return NULL;
}

/* 把池中的空闲的元素都给释放掉 */
/*
 * This function frees whatever can be freed in pool <pool>.
 */
void pool_clear(pool_head_p pool)
{
  void *temp, *next;
  if (!pool)
    return;

	pthread_mutex_lock(&(pool->lock));
  next = pool->free_list;
  pool->free_list=NULL;
  pool->allocated=pool->used;
  pthread_mutex_unlock(&(pool->lock));
  
  while(next){
		//pool池中的每一个内存快的前四个字节都是一个指针，其指向下一个内存块，
		//所以可以将temp转成一个二级指针获取到下下个内存块的地址
    temp = next;
    next = *(void **)temp;
    free(temp);
  }
	
	return;
}

/* 把池中非必要的元素给释放掉 */
/*
 * This function frees whatever can be freed in all pools, but respecting
 * the minimum thresholds imposed by owners. It takes care of avoiding
 * recursion because it may be called from a signal handler.
 */
void pool_flush_nonessential(mem_pools_p mpools)
{
  static int recurse;
  struct pool_head *entry;
  void *temp, *next;

  if(recurse++)
    goto out;

	pthread_rwlock_rdlock(&(mpools->rwlock));
  list_for_each_entry(entry, &(mpools->pools), list) {
  	pthread_mutex_lock(&(entry->lock));
    next = entry->free_list;
    while (next && entry->allocated > entry->minavail && entry->allocated > entry->used){
      temp = next;
      next = *(void **)temp;
      entry->allocated--;
      free(temp);
    }
    entry->free_list = next;
    pthread_mutex_unlock(&(entry->lock));
  }
  pthread_rwlock_unlock(&(mpools->rwlock));
 	out:
   	recurse--;
   
 	return;
}

/* 动态分配一个 pool 元素大小的内存空间 */
/* Allocate a new entry for pool <pool>, and return it for immediate use.
 * NULL is returned if no memory is available for a new creation. A call
 * to the garbage collector is performed before returning NULL.
 */
void *pool_alloc(mem_pools_p mpools, pool_head_p pool)
{
  void *ret;

	pthread_mutex_lock(&(pool->lock));
	//判断池是否可用
	if(pool->flags==0){
		pthread_mutex_unlock(&(pool->lock));
		return NULL;
	}
	//判断是否还有空闲内存块
	if((ret=pool->free_list)!=NULL){
		pool->free_list = *(void **)pool->free_list; 
  	pool->used++; 
  	pthread_mutex_unlock(&(pool->lock));
  	return ret;
	}
	//判断是否超过池的内存块数量极限
  if(pool->limit && (pool->allocated >= pool->limit)){
  	pthread_mutex_unlock(&(pool->lock));
    return NULL;
  }
  pool->allocated++;
  pool->used++;
  pthread_mutex_unlock(&(pool->lock));
  //动态分配新的内存块
  ret = calloc(1, pool->size);
  if(!ret){
    pool_flush_nonessential(mpools);
    ret = calloc(1, pool->size);
    if(!ret){
    	pthread_mutex_lock(&(pool->lock));
  		pool->allocated--;
  		pool->used--;
  		pthread_mutex_unlock(&(pool->lock));
      return NULL;
    }
  }
  
  return ret;
}

int pool_free(pool_head_p pool, void *mem_chunk)
{
	if(mem_chunk==NULL)
		return 0;
		
	pthread_mutex_lock(&(pool->lock));
	//判断池是否可,若不可用,则内存回收错误
	if(pool->flags==0){
		pthread_mutex_unlock(&(pool->lock));
		return -1;
	}
	
	*(void **)mem_chunk = (void *)pool->free_list;    
	pool->free_list = (void *)mem_chunk;    
	pool->used--;
	pthread_mutex_unlock(&(pool->lock));
	
	return 0;
}
