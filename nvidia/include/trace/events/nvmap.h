/*
 * include/trace/events/nvmap.h
 *
 * NvMap event logging to ftrace.
 *
 * Copyright (c) 2012-2022, NVIDIA CORPORATION. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM nvmap

#if !defined(_TRACE_NVMAP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_NVMAP_H

#include <linux/nvmap.h>
#include <linux/dma-buf.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

struct nvmap_handle;
struct nvmap_handle_ref;
struct nvmap_client;

DECLARE_EVENT_CLASS(nvmap,
	TP_PROTO(struct nvmap_client *client, const char *name),
	TP_ARGS(client, name),
	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__string(sname, name)
	),
	TP_fast_assign(
		__entry->client = client;
		__assign_str(sname, name)
	),
	TP_printk("client=%p, name=%s",
		__entry->client, __get_str(sname))
);

DEFINE_EVENT(nvmap, nvmap_open,
	TP_PROTO(struct nvmap_client *client, const char *name),
	TP_ARGS(client, name)
);

DEFINE_EVENT(nvmap, nvmap_release,
	TP_PROTO(struct nvmap_client *client, const char *name),
	TP_ARGS(client, name)
);

TRACE_EVENT(nvmap_create_handle,
	TP_PROTO(struct nvmap_client *client,
		 const char *name,
		 struct nvmap_handle *h,
		 u32 size,
		 struct nvmap_handle_ref *ref
	),

	TP_ARGS(client, name, h, size, ref),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__string(sname, name)
		__field(struct nvmap_handle *, h)
		__field(u32, size)
		__field(struct nvmap_handle_ref *, ref)
	),

	TP_fast_assign(
		__entry->client = client;
		__assign_str(sname, name)
		__entry->h = h;
		__entry->size = size;
		__entry->ref = ref;
	),

	TP_printk("client=%p, name=%s, handle=%p, size=%d, ref=%p",
		__entry->client, __get_str(sname),
		__entry->h, __entry->size, __entry->ref)
);

TRACE_EVENT(nvmap_alloc_handle,
	TP_PROTO(struct nvmap_client *client,
		 struct nvmap_handle *handle,
		 size_t size,
		 u32 heap_mask,
		 u32 align,
		 u32 flags,
		 u64 total,
		 u64 alloc
	),

	TP_ARGS(client, handle, size, heap_mask, align, flags, total, alloc),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(struct nvmap_handle *, handle)
		__field(size_t, size)
		__field(u32, heap_mask)
		__field(u32, align)
		__field(u32, flags)
		__field(u64, total)
		__field(u64, alloc)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->handle = handle;
		__entry->size = size;
		__entry->heap_mask = heap_mask;
		__entry->align = align;
		__entry->flags = flags;
		__entry->total = total;
		__entry->alloc = alloc;
	),

	TP_printk("client=%p, id=0x%p, size=%zu, heap_mask=0x%x, align=%d, flags=0x%x, total=%llu, alloc=%llu",
		__entry->client, __entry->handle, __entry->size,
		__entry->heap_mask, __entry->align, __entry->flags,
		(unsigned long long)__entry->total,
		(unsigned long long)__entry->alloc)
);


DECLARE_EVENT_CLASS(nvmap_handle_summary,
	TP_PROTO(struct nvmap_client *client, pid_t pid, u32 dupes,
		 struct nvmap_handle *handle, u32 share, u64 base,
		 size_t size, u32 flags, u32 tag, const char *tag_name),
	TP_ARGS(client, pid, dupes, handle, share, base, size, flags, tag, tag_name),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(pid_t, pid)
		__field(u32, dupes)
		__field(struct nvmap_handle *, handle)
		__field(u32, share)
		__field(u64, base)
		__field(size_t, size)
		__field(u32, flags)
		__field(u32, tag)
		__string(tag_name, tag_name)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->pid = pid;
		__entry->dupes = dupes;
		__entry->handle = handle;
		__entry->share = share;
		__entry->base = base;
		__entry->size = size;
		__entry->flags = flags;
		__entry->tag = tag;
		__assign_str(tag_name, tag_name)
	),

	TP_printk("client=0x%p pid=%d dupes=%u handle=0x%p share=%u base=%llx size=%zu flags=0x%x tag=0x%x %s",
		__entry->client,
		__entry->pid,
		__entry->dupes,
		__entry->handle,
		__entry->share,
		__entry->base,
		__entry->size,
		__entry->flags,
		__entry->tag,
		__get_str(tag_name)
	)
);

DEFINE_EVENT(nvmap_handle_summary,
	nvmap_alloc_handle_done,
	TP_PROTO(struct nvmap_client *client, pid_t pid, u32 dupes,
		 struct nvmap_handle *handle, u32 share, u64 base,
		 size_t size, u32 flags, u32 tag, const char *tag_name),
	TP_ARGS(client, pid, dupes, handle, share, base, size, flags, tag, tag_name)
);


DEFINE_EVENT(nvmap_handle_summary,
	nvmap_duplicate_handle,
	TP_PROTO(struct nvmap_client *client, pid_t pid, u32 dupes,
		 struct nvmap_handle *handle, u32 share, u64 base,
		 size_t size, u32 flags, u32 tag, const char *tag_name),
	TP_ARGS(client, pid, dupes, handle, share, base, size, flags, tag, tag_name)
);

DEFINE_EVENT(nvmap_handle_summary,
	nvmap_free_handle,
	TP_PROTO(struct nvmap_client *client, pid_t pid, u32 dupes,
		 struct nvmap_handle *handle, u32 share, u64 base,
		 size_t size, u32 flags, u32 tag, const char *tag_name),
	TP_ARGS(client, pid, dupes, handle, share, base, size, flags, tag, tag_name)
);

DEFINE_EVENT(nvmap_handle_summary,
	nvmap_destroy_handle,
	TP_PROTO(struct nvmap_client *client, pid_t pid, u32 dupes,
		 struct nvmap_handle *handle, u32 share, u64 base,
		 size_t size, u32 flags, u32 tag, const char *tag_name),
	TP_ARGS(client, pid, dupes, handle, share, base, size, flags, tag, tag_name)
);

TRACE_EVENT(nvmap_cache_maint,
	TP_PROTO(struct nvmap_client *client,
		 struct nvmap_handle *h,
		 ulong start,
		 ulong end,
		 u32 op,
		 size_t size
	),

	TP_ARGS(client, h, start, end, op, size),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(struct nvmap_handle *, h)
		__field(ulong, start)
		__field(ulong, end)
		__field(u32, op)
		__field(size_t, size)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->h = h;
		__entry->start = start;
		__entry->end = end;
		__entry->op = op;
		__entry->size = size;
	),

	TP_printk("client=%p, h=%p, start=0x%lx, end=0x%lx, op=%d, size=%zu",
		__entry->client, __entry->h, __entry->start,
		__entry->end, __entry->op, __entry->size)
);

TRACE_EVENT(nvmap_cache_flush,
	TP_PROTO(size_t size,
		 u64 alloc_rq,
		 u64 total_rq,
		 u64 total_done
	),

	TP_ARGS(size, alloc_rq, total_rq, total_done),

	TP_STRUCT__entry(
		__field(size_t, size)
		__field(u64, alloc_rq)
		__field(u64, total_rq)
		__field(u64, total_done)
	),

	TP_fast_assign(
		__entry->size = size;
		__entry->alloc_rq = alloc_rq;
		__entry->total_rq = total_rq;
		__entry->total_done = total_done;
	),

	TP_printk("size=%zu, alloc_rq=%llu, total_rq=%llu, total_done=%llu",
		__entry->size,
		(unsigned long long)__entry->alloc_rq,
		(unsigned long long)__entry->total_rq,
		(unsigned long long)__entry->total_done)
);

TRACE_EVENT(nvmap_map_into_caller_ptr,
	TP_PROTO(struct nvmap_client *client,
		 struct nvmap_handle *h,
		 u32 offset,
		 u32 length,
		 u32 flags
	),

	TP_ARGS(client, h, offset, length, flags),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(struct nvmap_handle *, h)
		__field(u32, offset)
		__field(u32, length)
		__field(u32, flags)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->h = h;
		__entry->offset = offset;
		__entry->length = length;
		__entry->flags = flags;
	),

	TP_printk("client=%p, h=%p, offset=%d, length=%d, flags=0x%x",
		__entry->client, __entry->h, __entry->offset,
		__entry->length, __entry->flags)
);

TRACE_EVENT(nvmap_ioctl_rw_handle,
	TP_PROTO(struct nvmap_client *client,
		 struct nvmap_handle *h,
		 u32 is_read,
		 u32 offset,
		 unsigned long addr,
		 u32 mem_stride,
		 u32 user_stride,
		 u32 elem_size,
		 u32 count
	),

	TP_ARGS(client, h, is_read, offset, addr, mem_stride,
		user_stride, elem_size, count),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(struct nvmap_handle *, h)
		__field(u32, is_read)
		__field(u32, offset)
		__field(unsigned long, addr)
		__field(u32, mem_stride)
		__field(u32, user_stride)
		__field(u32, elem_size)
		__field(u32, count)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->h = h;
		__entry->is_read = is_read;
		__entry->offset = offset;
		__entry->addr = addr;
		__entry->mem_stride = mem_stride;
		__entry->user_stride = user_stride;
		__entry->elem_size = elem_size;
		__entry->count = count;
	),

	TP_printk("client=%p, h=%p, is_read=%d, offset=%d, addr=0x%lx,"
		"mem_stride=%d, user_stride=%d, elem_size=%d, count=%d",
		__entry->client, __entry->h, __entry->is_read, __entry->offset,
		__entry->addr, __entry->mem_stride, __entry->user_stride,
		__entry->elem_size, __entry->count)
);

TRACE_EVENT(nvmap_ioctl_pinop,
	TP_PROTO(struct nvmap_client *client,
		 u32 is_pin,
		 u32 count,
		 struct nvmap_handle **ids
	),

	TP_ARGS(client, is_pin, count, ids),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__field(u32, is_pin)
		__field(u32, count)
		__field(struct nvmap_handle **, ids)
		__dynamic_array(struct nvmap_handle *, ids, count)
	),

	TP_fast_assign(
		__entry->client = client;
		__entry->is_pin = is_pin;
		__entry->count = count;
		__entry->ids = ids;
		memcpy(__get_dynamic_array(ids), ids,
		    sizeof(struct nvmap_handle *) * count);
	),

	TP_printk("client=%p, is_pin=%d, count=%d, ids=[%s]",
		__entry->client, __entry->is_pin, __entry->count,
		__print_hex(__get_dynamic_array(ids), __entry->ids ?
			    sizeof(struct nvmap_handle *) * __entry->count : 0))
);

DECLARE_EVENT_CLASS(handle_get_put,
	TP_PROTO(struct nvmap_handle *handle, u32 ref_count),

	TP_ARGS(handle, ref_count),

	TP_STRUCT__entry(
		__field(struct nvmap_handle *, handle)
		__field(u32, ref_count)
	),

	TP_fast_assign(
		__entry->handle = handle;
		__entry->ref_count = ref_count;
	),

	TP_printk("ref=%u handle=%p",
		__entry->ref_count,
		__entry->handle
	)
);

DEFINE_EVENT(handle_get_put, nvmap_handle_get,
	TP_PROTO(struct nvmap_handle *handle, u32 ref_count),
	TP_ARGS(handle, ref_count)
);

DEFINE_EVENT(handle_get_put, nvmap_handle_put,
	TP_PROTO(struct nvmap_handle *handle, u32 ref_count),
	TP_ARGS(handle, ref_count)
);

DECLARE_EVENT_CLASS(pin_unpin,
	TP_PROTO(struct nvmap_client *client,
		 const char *name,
		 struct nvmap_handle *h,
		 u32 pin_count
	),

	TP_ARGS(client, name, h, pin_count),

	TP_STRUCT__entry(
		__field(struct nvmap_client *, client)
		__string(sname, name)
		__field(struct nvmap_handle *, h)
		__field(u32, pin_count)
	),

	TP_fast_assign(
		__entry->client = client;
		__assign_str(sname, name)
		__entry->h = h;
		__entry->pin_count = pin_count;
	),

	TP_printk("client=%p, name=%s, h=%p, pin_count=%d",
		__entry->client, __get_str(sname),
		__entry->h, __entry->pin_count)
);

DEFINE_EVENT(pin_unpin, nvmap_pin,
	TP_PROTO(struct nvmap_client *client,
		 const char *name,
		 struct nvmap_handle *h,
		 u32 pin_count
	),
	TP_ARGS(client, name, h, pin_count)
);

DEFINE_EVENT(pin_unpin, nvmap_unpin,
	TP_PROTO(struct nvmap_client *client,
		 const char *name,
		 struct nvmap_handle *h,
		 u32 pin_count
	),
	TP_ARGS(client, name, h, pin_count)
);

DEFINE_EVENT(pin_unpin, handle_unpin_error,
	TP_PROTO(struct nvmap_client *client,
		 const char *name,
		 struct nvmap_handle *h,
		 u32 pin_count
	),
	TP_ARGS(client, name, h, pin_count)
);

/*
 * Nvmap dmabuf events
 */
DECLARE_EVENT_CLASS(nvmap_dmabuf_2,
	TP_PROTO(struct dma_buf *dbuf,
		 struct device *dev
	),

	TP_ARGS(dbuf, dev),

	TP_STRUCT__entry(
		__field(struct dma_buf *, dbuf)
		__string(name, dev_name(dev))
	),

	TP_fast_assign(
		__entry->dbuf = dbuf;
		__assign_str(name, dev_name(dev));
	),

	TP_printk("dmabuf=%p, device=%s",
		__entry->dbuf, __get_str(name)
	)
);

DECLARE_EVENT_CLASS(nvmap_dmabuf_1,
	TP_PROTO(struct dma_buf *dbuf),

	TP_ARGS(dbuf),

	TP_STRUCT__entry(
		__field(struct dma_buf *, dbuf)
	),

	TP_fast_assign(
		__entry->dbuf = dbuf;
	),

	TP_printk("dmabuf=%p",
		__entry->dbuf
	)
);

DEFINE_EVENT(nvmap_dmabuf_2, nvmap_dmabuf_attach,
	TP_PROTO(struct dma_buf *dbuf,
		 struct device *dev
	),
	TP_ARGS(dbuf, dev)
);

DEFINE_EVENT(nvmap_dmabuf_2, nvmap_dmabuf_detach,
	TP_PROTO(struct dma_buf *dbuf,
		 struct device *dev
	),
	TP_ARGS(dbuf, dev)
);

DEFINE_EVENT(nvmap_dmabuf_2, nvmap_dmabuf_map_dma_buf,
	TP_PROTO(struct dma_buf *dbuf,
		 struct device *dev
	),
	TP_ARGS(dbuf, dev)
);

DEFINE_EVENT(nvmap_dmabuf_2, nvmap_dmabuf_unmap_dma_buf,
	TP_PROTO(struct dma_buf *dbuf,
		 struct device *dev
	),
	TP_ARGS(dbuf, dev)
);

DEFINE_EVENT(nvmap_dmabuf_1, nvmap_dmabuf_mmap,
	TP_PROTO(struct dma_buf *dbuf),
	TP_ARGS(dbuf)
);

DEFINE_EVENT(nvmap_dmabuf_1, nvmap_dmabuf_vmap,
	TP_PROTO(struct dma_buf *dbuf),
	TP_ARGS(dbuf)
);

DEFINE_EVENT(nvmap_dmabuf_1, nvmap_dmabuf_vunmap,
	TP_PROTO(struct dma_buf *dbuf),
	TP_ARGS(dbuf)
);

DEFINE_EVENT(nvmap_dmabuf_1, nvmap_dmabuf_kmap,
	TP_PROTO(struct dma_buf *dbuf),
	TP_ARGS(dbuf)
);

DEFINE_EVENT(nvmap_dmabuf_1, nvmap_dmabuf_kunmap,
	TP_PROTO(struct dma_buf *dbuf),
	TP_ARGS(dbuf)
);

DECLARE_EVENT_CLASS(nvmap_dmabuf_cpu_access,
	TP_PROTO(struct dma_buf *dbuf,
		 size_t start,
		 size_t len),

	TP_ARGS(dbuf, start, len),

	TP_STRUCT__entry(
		__field(struct dma_buf *, dbuf)
		__field(size_t, start)
		__field(size_t, len)
	),

	TP_fast_assign(
		__entry->dbuf = dbuf;
		__entry->start = start;
		__entry->len = len;
	),

	TP_printk("dmabuf=%p, start=%zd len=%zd",
		  __entry->dbuf, __entry->start, __entry->len
	)
);

DEFINE_EVENT(nvmap_dmabuf_cpu_access, nvmap_dmabuf_begin_cpu_access,
	TP_PROTO(struct dma_buf *dbuf,
		 size_t start,
		 size_t len),
	TP_ARGS(dbuf, start, len)
);

DEFINE_EVENT(nvmap_dmabuf_cpu_access, nvmap_dmabuf_end_cpu_access,
	TP_PROTO(struct dma_buf *dbuf,
		 size_t start,
		 size_t len),
	TP_ARGS(dbuf, start, len)
);

DECLARE_EVENT_CLASS(nvmap_dmabuf_make_release,
	TP_PROTO(const char *cli,
		 struct nvmap_handle *h,
		 struct dma_buf *dbuf
	),

	TP_ARGS(cli, h, dbuf),

	TP_STRUCT__entry(
		__field(const char *, cli)
		__field(struct nvmap_handle *, h)
		__field(struct dma_buf *, dbuf)
	),

	TP_fast_assign(
		__entry->cli = cli;
		__entry->h = h;
		__entry->dbuf = dbuf;
	),

	TP_printk("cli=%s handle=%p dmabuf=%p",
		  __entry->cli, __entry->h, __entry->dbuf
	)
);

DEFINE_EVENT(nvmap_dmabuf_make_release, nvmap_make_dmabuf,
	TP_PROTO(const char *cli,
		 struct nvmap_handle *h,
		 struct dma_buf *dbuf
	),
	TP_ARGS(cli, h, dbuf)
);

DEFINE_EVENT(nvmap_dmabuf_make_release, nvmap_dmabuf_release,
	TP_PROTO(const char *cli,
		 struct nvmap_handle *h,
		 struct dma_buf *dbuf
	),
	TP_ARGS(cli, h, dbuf)
);

TRACE_EVENT(pp_clean_cache,
	TP_PROTO(u32 dirty_pages,
		 size_t cache_maint_th,
		 int cache_maint_by_set_ways
	),

	TP_ARGS(dirty_pages, cache_maint_th, cache_maint_by_set_ways),

	TP_STRUCT__entry(
		__field(u32, dirty_pages)
		__field(size_t, cache_maint_th)
		__field(int, cache_maint_by_set_ways)
	),

	TP_fast_assign(
		__entry->dirty_pages = dirty_pages;
		__entry->cache_maint_th = cache_maint_th >> PAGE_SHIFT;
		__entry->cache_maint_by_set_ways = cache_maint_by_set_ways;
	),

	TP_printk("dirty_pages=%u, cache_maint_th=%zu, cache_maint_by_set_ways=%d",
		__entry->dirty_pages, __entry->cache_maint_th,
		__entry->cache_maint_by_set_ways)
);

DECLARE_EVENT_CLASS(nvmap_get_list_page,
	TP_PROTO(u32 count),

	TP_ARGS(count),

	TP_STRUCT__entry(
		__field(u32, count)
	),

	TP_fast_assign(
		__entry->count = count;
	),

	TP_printk("pages left in list=%u", __entry->count)
);

DEFINE_EVENT(nvmap_get_list_page, get_zero_list_page,
	TP_PROTO(u32 count),
	TP_ARGS(count)
);

DEFINE_EVENT(nvmap_get_list_page, get_page_list_page,
	TP_PROTO(u32 count),
	TP_ARGS(count)
);

TRACE_EVENT(nvmap_pp_zero_pages,
	TP_PROTO(u32 count),

	TP_ARGS(count),

	TP_STRUCT__entry(
		__field(u32, count)
	),

	TP_fast_assign(
		__entry->count = count;
	),

	TP_printk("no. of pages zeroed=%u", __entry->count)
);

TRACE_EVENT(nvmap_pp_do_background_zero_pages,
	TP_PROTO(u32 inserted, u32 zeroed),

	TP_ARGS(inserted, zeroed),

	TP_STRUCT__entry(
		__field(u32, inserted)
		__field(u32, zeroed)
	),

	TP_fast_assign(
		__entry->inserted = inserted;
		__entry->zeroed = zeroed;
	),

	TP_printk("failed to insert %u no. of zeroed pages to page_list",
		__entry->zeroed - __entry->inserted)
);

TRACE_EVENT(nvmap_pp_alloc_locked,
	TP_PROTO(int force_alloc),

	TP_ARGS(force_alloc),

	TP_STRUCT__entry(
		__field(int, force_alloc)
	),

	TP_fast_assign(
		__entry->force_alloc = force_alloc;
	),

	TP_printk("allocated one page with force_alloc:%d", __entry->force_alloc)
);

TRACE_EVENT(nvmap_pp_alloc_lots,
	TP_PROTO(u32 alloced, u32 requested),

	TP_ARGS(alloced, requested),

	TP_STRUCT__entry(
		__field(u32, alloced)
		__field(u32, requested)
	),

	TP_fast_assign(
		__entry->alloced = alloced;
		__entry->requested = requested;
	),

	TP_printk("requested:%u alloced:%u\n",
		__entry->requested, __entry->alloced)
);

TRACE_EVENT(nvmap_pp_fill_zero_lots,
	TP_PROTO(u32 save_to_zero,
		 u32 to_zero,
		 u32 ret,
		 u32 nr
	),

	TP_ARGS(save_to_zero, to_zero, ret, nr),

	TP_STRUCT__entry(
		__field(u32, save_to_zero)
		__field(u32, to_zero)
		__field(u32, ret)
		__field(u32, nr)
	),

	TP_fast_assign(
		__entry->save_to_zero = save_to_zero;
		__entry->to_zero = to_zero;
		__entry->ret = ret;
		__entry->nr = nr;
	),

	TP_printk("inserted %u pages to zero list, freed %u pages, did not process %u pages",
		__entry->to_zero - __entry->save_to_zero,
		__entry->ret - (__entry->to_zero - __entry->save_to_zero),
		__entry->nr - __entry->ret)
);

TRACE_EVENT(refcount_get_handle_from_sci_ipc_id,
	TP_PROTO(struct nvmap_handle *handle,
		 struct dma_buf *dmabuf,
		 int handle_ref, const char *perm
	),

	TP_ARGS(handle, dmabuf, handle_ref, perm),

	TP_STRUCT__entry(
		__field(struct nvmap_handle *, handle)
		__field(struct dma_buf *, dmabuf)
		__field(int, handle_ref)
		__string(acc_perm, perm)
	),

	TP_fast_assign(
		__entry->handle = handle;
		__entry->dmabuf = dmabuf;
		__entry->handle_ref = handle_ref;
		__assign_str(acc_perm, perm);
	),

	TP_printk("handle=0x%p, dmabuf=0x%p, handle_ref=%d, perm=%s",
		   __entry->handle, __entry->dmabuf, __entry->handle_ref,
		   __get_str(acc_perm))
);

DECLARE_EVENT_CLASS(nvmap_refcount,
	TP_PROTO(struct nvmap_handle *handle, struct dma_buf *dmabuf, \
		 int handle_ref,  long dmabuf_ref, const char *perm),

	TP_ARGS(handle, dmabuf, handle_ref, dmabuf_ref, perm),

	TP_STRUCT__entry(
		__field(struct nvmap_handle *, handle)
		__field(struct dma_buf *, dmabuf)
		__field(int, handle_ref)
		__field(long, dmabuf_ref)
		__string(acc_perm, perm)
	),

	TP_fast_assign(
		__entry->handle = handle;
		__entry->dmabuf = dmabuf;
		__entry->handle_ref = handle_ref;
		__entry->dmabuf_ref = dmabuf_ref;
		__assign_str(acc_perm, perm);
	),

	TP_printk("handle=0x%p, dmabuf=0x%p, handle_ref=%d, dmabuf_ref=%ld, perm=%s",
		   __entry->handle, __entry->dmabuf, __entry->handle_ref,
		   __entry->dmabuf_ref, __get_str(acc_perm))
);

#define NVMAPREFCOUNTEVENT(ev) DEFINE_EVENT(nvmap_refcount, ev, \
	TP_PROTO(struct nvmap_handle *handle, struct dma_buf *dmabuf, \
		 int handle_ref,  long dmabuf_ref, const char *perm), \
	TP_ARGS(handle, dmabuf, handle_ref, dmabuf_ref, perm) \
)

NVMAPREFCOUNTEVENT(refcount_create_handle);
NVMAPREFCOUNTEVENT(refcount_create_handle_from_va);
NVMAPREFCOUNTEVENT(refcount_create_handle_from_fd);
NVMAPREFCOUNTEVENT(refcount_getfd);
NVMAPREFCOUNTEVENT(refcount_alloc);
NVMAPREFCOUNTEVENT(refcount_get_sci_ipc_id);
NVMAPREFCOUNTEVENT(refcount_create_handle_from_sci_ipc_id);
NVMAPREFCOUNTEVENT(refcount_dup_handle);
NVMAPREFCOUNTEVENT(refcount_free_handle);

#undef NVMAPREFCOUNTEVENT

#endif /* _TRACE_NVMAP_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
