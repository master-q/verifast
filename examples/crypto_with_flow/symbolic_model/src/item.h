#ifndef ITEM_H
#define ITEM_H

#include "general.h"

struct item
{
  int size;
  char* content;
};

/*@

predicate item_constraints(item i, list<char> cs, predicate(item) pub);

predicate item(struct item *item, item i, predicate(item) pub) =
  item != 0 &*&
  item->size |-> ?size &*&
    size > MINIMAL_STRING_SIZE &*& size <= INT_MAX &*&
  item->content |-> ?content &*&
    crypto_chars(secret, content, size, ?cs) &*&
    content >= (char *)0 &*& content + size <= (char *)UINTPTR_MAX &*&
    size == length(cs) &*& malloc_block_chars(content, size) &*&
  malloc_block_item(item) &*&

  [_]item_constraints(i, cs, pub)
;

lemma_auto void non_zero_items()
  requires item(?item, ?i, ?pub);
  ensures  item(item, i, pub) &*& item != 0;
{
  open item(item, i, pub);
  close item(item, i, pub);
}

@*/

#endif
