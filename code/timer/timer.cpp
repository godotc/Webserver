#include "timer.h"
#include <cassert>


void
HeapTimer::adjust (int id, int timeout)
{
    assert (!heap_.empty () && ref_.count (id) > 0);

    heap_[ref_[id]].expires = Clock::now () + MS (timeout);
}