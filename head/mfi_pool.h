#ifndef _MFI_POOL_HEADER_
#define _MFI_POOL_HEADER_

#include "mfi_list.h"
#include "mfiapi.h"

#define DEF_MEM_PAGE    128
#define MEM_F_SHARED    0x1                /* ��ʾ��Ӧ�ĳ��������� */

/* ÿ���ص�������Ϣ */
typedef struct pool_head {
	void                     **free_list;     //�����ڴ���������ÿ���ڴ�����ǰ�ĸ��ֽڴ洢�¸��洢���ĵ�ַ
  MfiUInt32                used;            /* how many chunks are currently in use ������ȥ����ʹ�õĿ�����*/
  MfiUInt32                allocated;        /* how many chunks have been allocated �ѷ����Ŀ���������������ʹ�ú����ڳ��п���������*/
  MfiUInt32                limit;            /* hard limit on the number of chunks ����������������,0Ϊ����*/
  MfiUInt32                minavail;        /* how many chunks are expected to be used �ͷŲ���Ҫ��ʱ�������ĺ��ʵ���С����*/
  MfiUInt32                size;            /* chunk size */
  MfiUInt32                flags;            /* MEM_F_* */
//  MfiUInt32                users;            /* number of pools sharing this zone ��¼����ʹ��ĳһ�ߴ��ص��û�����*/
  // pthread_mutex_t       lock;
  struct list_head         list;            //ʹ��linux�Դ���list����֯���д�С���͵�pool
}pool_head,*pool_head_p;

typedef struct mem_pools{
  MfiUInt32                limit;            /* hard limit on the number of chunks */
  MfiUInt32                minavail;        /* how many chunks are expected to be used */
  MfiUInt32                mempage;
	struct list_head         pools;
	// pthread_rwlock_t         rwlock;
}mem_pools,*mem_pools_p;

/* �ش��� */
/* Try to find an existing shared pool with the same characteristics and
 * returns it, otherwise creates this one. NULL is returned if no memory
 * is available for a new creation.
 */

MfiStatus pool_create(mem_pools_p mpools, MfiUInt32 size, pool_head_p* pool_head_r);

/* ������ */
/*
 * This function destroys a pool by freeing it completely, unless it's still
 * in use. This should be called only under extreme circumstances. It always
 * returns NULL if the resulting pool is empty, easing the clearing of the old
 * pointer, otherwise it returns the pool.
 */
extern void* pool_destroy(mem_pools_p mpools, pool_head_p pool);

/* �ѳ��еĿ��е�Ԫ�ض����ͷŵ� */
/*
 * This function frees whatever can be freed in pool <pool>.
 */
extern void pool_clear(pool_head_p pool);

/* �ѳ��зǱ�Ҫ��Ԫ�ظ��ͷŵ� */
/*
 * This function frees whatever can be freed in all pools, but respecting
 * the minimum thresholds imposed by owners. It takes care of avoiding
 * recursion because it may be called from a signal handler.
 */
extern void pool_flush_nonessential(mem_pools_p mpools);

/* ��̬����һ�� pool Ԫ�ش�С���ڴ��ռ� */
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
//�ӳ��л�ȡ��

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
//��һ�������յ�����
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
