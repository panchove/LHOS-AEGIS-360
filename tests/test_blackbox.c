// test_blackbox.c – QA/ownership para logging/blackbox (Sprint 02, ODIN)
#include "blackbox.h"
#include <assert.h>
#include <stdio.h>

void test_enqueue_dequeue() {
    const uint8_t testdata[8] = {0,1,2,3,4,5,6,7};
    assert(blackbox_enqueue(testdata, 8) == 0);
    assert(blackbox_get_stats() == 1);
    uint8_t out[32]; size_t olen=sizeof(out);
    assert(blackbox_read_next(out, &olen) == 0);
    assert(olen == 8);
    for (int i=0;i<8;++i) assert(out[i]==i);
}
void test_full_limit() {
    uint8_t d[1]={0xFF};
    int fills=0;
    while (blackbox_enqueue(d,1)==0) ++fills;
    assert(fills > 0 && fills <= 16);
}
int main() {
    printf("[QA] blackbox: test enqueue/dequeue\n");
    test_enqueue_dequeue();
    printf("[QA] blackbox: test full limit\n");
    test_full_limit();
    puts("[OK] blackbox QA tests PASSED");
    return 0;
}
