#pragma once
#include "util.h"
#include "ticker.h"

void tickers_init();
void ticker_add(ticker* ticker);
void ticker_remove(ticker* ticker);
void tickers_run();

