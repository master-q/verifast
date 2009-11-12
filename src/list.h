#ifndef LIST_H
#define LIST_H

#include "prelude_core.h"

/*@

inductive list<t> = nil | cons(t, list<t>);

fixpoint t head<t>(list<t> xs) {
    switch (xs) {
        case nil: return default<t>;
        case cons(x, xs0): return x;
    }
}

fixpoint list<t> tail<t>(list<t> xs) {
    switch (xs) {
        case nil: return nil;
        case cons(x, xs0): return xs0;
    }
}

fixpoint int length<t>(list<t> xs) {
    switch (xs) {
        case nil: return 0;
        case cons(x, xs0): return 1 + length(xs0);
    }
}

lemma_auto void length_nonnegative<t>(list<t> xs);
    requires true;
    ensures length(xs) >= 0;

fixpoint list<t> append<t>(list<t> xs, list<t> ys) {
    switch (xs) {
        case nil: return ys;
        case cons(x, xs0): return cons(x, append(xs0, ys));
    }
}

lemma_auto void append_nil<t>(list<t> xs);
    requires true;
    ensures append(xs, nil) == xs;

lemma void append_assoc<t>(list<t> xs, list<t> ys, list<t> zs);
    requires true;
    ensures append(append(xs, ys), zs) == append(xs, append(ys, zs));
    
lemma_auto(length(append<t>(xs, ys))) void length_append<t>(list<t> xs, list<t> ys);
  requires true;
  ensures length(append(xs, ys)) == length(xs) + length(ys);


fixpoint list<t> reverse<t>(list<t> xs) {
    switch (xs) {
        case nil: return nil;
        case cons(x, xs0): return append(reverse(xs0), cons(x, nil));
    }
}

lemma void reverse_append<t>(list<t> xs, list<t> ys);
    requires true;
    ensures reverse(append(xs, ys)) == append(reverse(ys), reverse(xs));

lemma_auto void reverse_reverse<t>(list<t> xs);
    requires true;
    ensures reverse(reverse(xs)) == xs;

fixpoint bool mem<t>(t x, list<t> xs) {
    switch (xs) {
        case nil: return false;
        case cons(x0, xs0): return x == x0 || mem(x, xs0);
    }
}

fixpoint t nth<t>(int n, list<t> xs) {
    switch (xs) {
        case nil: return default<t>;
        case cons(x, xs0): return n == 0 ? x : nth(n - 1, xs0);
    }
}

lemma_auto(mem(nth(n, xs), xs)) void mem_nth<t>(int n, list<t> xs);
    requires 0 <= n && n < length(xs);
    ensures mem(nth(n, xs), xs) == true;

fixpoint bool distinct<t>(list<t> xs) {
    switch (xs) {
        case nil: return true;
        case cons(x, xs0): return !mem(x, xs0) && distinct(xs0);
    }
}

fixpoint list<t> take<t>(int n, list<t> xs) {
    switch (xs) {
        case nil: return nil;
        case cons(x, xs0): return n == 0 ? nil : cons(x, take(n - 1, xs0));
    }
}

lemma_auto void take_0<t>(list<t> xs);
    requires true;
    ensures take(0, xs) == nil;

lemma_auto(take(length(xs), xs)) void take_length<t>(list<t> xs);
    requires true;
    ensures take(length(xs), xs) == xs;

lemma_auto(length(take(n, xs))) void length_take<t>(int n, list<t> xs);
    requires 0 <= n && n <= length(xs);
    ensures length(take(n, xs)) == n;

lemma_auto(nth(i, take(n, xs))) void nth_take<t>(int i, int n, list<t> xs);
    requires 0 <= i && i < n && n <= length(xs);
    ensures nth(i, take(n, xs)) == nth(i, xs);

fixpoint list<t> drop<t>(int n, list<t> xs) {
    switch (xs) {
        case nil: return nil;
        case cons(x, xs0): return n == 0 ? xs : drop(n - 1, xs0);
    }
}

lemma_auto void drop_0<t>(list<t> xs);
    requires true;
    ensures drop(0, xs) == xs;

lemma void drop_n_plus_one<t>(int n, list<t> xs);
    requires 0 <= n &*& n < length(xs);
    ensures drop(n, xs) == cons(nth(n, xs), drop(n + 1, xs));

lemma_auto(drop(length(xs), xs)) void drop_length<t>(list<t> xs);
    requires true;
    ensures drop(length(xs), xs) == nil;

lemma_auto(length(drop(n, xs))) void length_drop<t>(int n, list<t> xs);
    requires 0 <= n && n <= length(xs);
    ensures length(drop(n, xs)) == length(xs) - n;

lemma_auto(drop(n, take(n, xs))) void drop_n_take_n<t>(int n, list<t> xs);
    requires true;
    ensures drop(n, take(n, xs)) == nil;

fixpoint list<t> remove<t>(t x, list<t> xs) {
    switch (xs) {
        case nil: return nil;
        case cons(x0, xs0): return x0 == x ? xs0 : cons(x0, remove(x, xs0));
    }
}

fixpoint list<t> remove_nth<t>(int n, list<t> xs) {
    switch(xs) {
        case nil: return nil;
        case cons(h, t): return n == 0 ? t : cons(h, remove_nth(n - 1, t));
    }
}

lemma_auto(append(take(n, xs), tail(drop(n, xs)))) void drop_take_remove_nth<t>(list<t> xs, int n);
  requires 0<=n && n < length(xs);
  ensures append(take(n, xs), tail(drop(n, xs))) == remove_nth(n, xs);

fixpoint int index_of<t>(t x, list<t> xs) {
    switch (xs) {
        case nil: return -1;
        case cons(x0, xs0): return x0 == x ? 0 : 1 + index_of(x, xs0);
    }
}

lemma void mem_index_of<t>(t x, list<t> xs);
    requires mem(x, xs) == true;
    ensures mem(x, xs) == (0 <= index_of(x, xs)) &*& index_of(x, xs) <= length(xs);

predicate foreach<t>(list<t> xs, predicate(t) p) =
    switch (xs) {
        case nil: return true;
        case cons(x, xs0): return p(x) &*& foreach(xs0, p);
    };

lemma void foreach_remove<t>(t x, list<t> xs);
    requires foreach(xs, ?p) &*& mem(x, xs) == true;
    ensures foreach(remove(x, xs), p) &*& p(x);

lemma void foreach_unremove<t>(t x, list<t> xs);
    requires foreach(remove(x, xs), ?p) &*& mem(x, xs) == true &*& p(x);
    ensures foreach(xs, p);

lemma void foreach_append<t>(list<t> xs, list<t> ys);
    requires foreach(xs, ?p) &*& foreach(ys, p);
    ensures foreach(append(xs, ys), p);

@*/

#endif