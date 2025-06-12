#pragma once
#include "util.h"

typedef void (*actionf)(void*);

//both end and start of doubly linked list of tickers
typedef struct ticker_s
{
    struct ticker_s* prev;
    struct ticker_s* next;
    actionf function;

} ticker;

void tickers_init();
void ticker_add(ticker* ticker);
void ticker_remove(ticker* ticker);
void tickers_run();

