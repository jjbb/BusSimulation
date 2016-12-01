#include "mfi_pool.h"
#include "pthread.h"
#include <stdlib.h>

/* ��ʼ��һ���ڴ�� */
//struct list_head��linux�ṩ������ṹ��ֻ����ָ��ýṹ��next��prew����ָ���Ա
//LIST_HEAD_INIT��Linux�ṩ��һ���꣬��list_head�ṹ��ʼ��Ϊһ��next��prewָ�����������

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

/* Ϊһ���ڴ�ش����³ߴ�ĳ� */
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
	//�÷����ļ������ǣ�����page��sizeΪpage.������page�ı�������ռ�
	align=mpools->mempage;
  size = (size + align - 1) & -align;

  start = &(mpools->pools);
  pool = NULL;
  
  pthread_rwlock_rdlock(&(mpools->rwlock));
	//list_for_each_entry��һ���꣬����ѭ�����ذ���list_head������ÿһ���ڵ��ָ�뵽entry
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
		//calloc�ܷ���n*size�Ŀռ䣬����ʼ��
    pool = calloc(1, sizeof(*pool));
    if (!pool)
      return MFI_ERROR_ALLOC;

    pool->size = size;
    pool->limit=mpools->limit;
    pool->minavail=mpools->minavail;
    pool->flags=1;

		pthread_rwlock_wrlock(&(mpools->rwlock));
		//���ĳ�ִ�С���͵ĳ�
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

/* ������ */
void* pool_destroy(mem_pools_p mpools, pool_head_p pool)
{
  if(pool)
  {
    pool_clear(pool);            // �뿴���еĿ��е�Ԫ��
    
    pthread_mutex_lock(&(pool->lock));
    if(pool->used){
    	pthread_mutex_unlock(&(pool->lock));
      return pool;
    }
		pool->flags=0;
		pthread_mutex_unlock(&(pool->lock));
    pthread_rwlock_wrlock(&(mpools->rwlock));
    list_del(&pool->list);    // �� pools ������ɾ��
    pthread_rwlock_unlock(&(mpools->rwlock));
    free(pool);                // �� pool �ṹ��ռ�õ��ڴ���ͷ���
    
 /*   pool->users--;
    if (!pool->users)
    {
      list_del(&pool->list);    // �� pools ������ɾ��
      free(pool);                // �� pool �ṹ��ռ�õ��ڴ���ͷ���
    }
    */
  }
  return NULL;
}

/* �ѳ��еĿ��е�Ԫ�ض����ͷŵ� */
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
		//pool���е�ÿһ���ڴ���ǰ�ĸ��ֽڶ���һ��ָ�룬��ָ����һ���ڴ�飬
		//���Կ��Խ�tempת��һ������ָ���ȡ�����¸��ڴ��ĵ�ַ
    temp = next;
    next = *(void **)temp;
    free(temp);
  }
	
	return;
}

/* �ѳ��зǱ�Ҫ��Ԫ�ظ��ͷŵ� */
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

/* ��̬����һ�� pool Ԫ�ش�С���ڴ�ռ� */
/* Allocate a new entry for pool <pool>, and return it for immediate use.
 * NULL is returned if no memory is available for a new creation. A call
 * to the garbage collector is performed before returning NULL.
 */
void *pool_alloc(mem_pools_p mpools, pool_head_p pool)
{
  void *ret;

	pthread_mutex_lock(&(pool->lock));
	//�жϳ��Ƿ����
	if(pool->flags==0){
		pthread_mutex_unlock(&(pool->lock));
		return NULL;
	}
	//�ж��Ƿ��п����ڴ��
	if((ret=pool->free_list)!=NULL){
		pool->free_list = *(void **)pool->free_list; 
  	pool->used++; 
  	pthread_mutex_unlock(&(pool->lock));
  	return ret;
	}
	//�ж��Ƿ񳬹��ص��ڴ����������
  if(pool->limit && (pool->allocated >= pool->limit)){
  	pthread_mutex_unlock(&(pool->lock));
    return NULL;
  }
  pool->allocated++;
  pool->used++;
  pthread_mutex_unlock(&(pool->lock));
  //��̬�����µ��ڴ��
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
	//�жϳ��Ƿ��,��������,���ڴ���մ���
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
