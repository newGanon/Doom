#pragma once

//both end and start of doubly linked list of tickers

typedef void (*actionf)(void*);

typedef struct ticker_s
{
    struct ticker_s* prev;
    struct ticker_s* next;
    actionf function;

} ticker;