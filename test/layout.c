#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>

typedef struct hashmap_i
{
  size_t k_size;
  size_t v_size;
  size_t k_align;
  size_t v_align;
} hashmap_i;

static inline size_t
node_size (const hashmap_i *info)
{
  const size_t m_align
      = info->k_align > info->v_align ? info->k_align : info->v_align;
  size_t ret = sizeof (char) + info->k_size + info->v_size;
  if (ret % m_align)
    ret += m_align - ret % m_align;
  return ret;
}

static inline size_t
key_offset (const hashmap_i *info)
{
  size_t ret = 0 + sizeof (char);
  if (ret % info->k_align)
    ret += info->k_align - ret % info->k_align;
  return ret;
}

static inline size_t
val_offset (const hashmap_i *info)
{
  size_t ret = key_offset (info) + info->k_size;
  if (ret % info->v_align)
    ret += info->v_align - ret % info->v_align;
  return ret;
}

struct test
{
  char state;
  char key[17];
  long double val[2];
};

int
main (void)
{
  hashmap_i info = { .k_size = sizeof (char) * 17,
                     .v_size = sizeof (long double) * 2,
                     .k_align = alignof (char),
                     .v_align = alignof (long double) };

  printf ("size: %lu, key_off: %lu, val_off: %lu\n", node_size (&info),
          key_offset (&info), val_offset (&info));
}
