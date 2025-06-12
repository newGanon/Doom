#include "ticker.h"
#include "util.h"
//head and tail of tickerlist

ticker tickercap;

void tickers_init() {
	tickercap.prev = tickercap.next = &tickercap;
}

void ticker_add(ticker* ticker) {
	tickercap.prev->next = ticker;
	ticker->next = &tickercap;
	ticker->prev = tickercap.prev;
	tickercap.prev = ticker;
}

void ticker_remove(ticker* ticker) {
	if (ticker->next && ticker->prev) {
		ticker->next->prev = ticker->prev;
		ticker->prev->next = ticker->next;
		ticker->function = (actionf)(-1);
	}
}

void tickers_run() {
	ticker* currentticker;
	currentticker = tickercap.next;
	while (currentticker != &tickercap) {
		ticker* nextticker = currentticker->next;
		if (currentticker->function == (actionf)(-1)) {
			ticker_remove(currentticker);
		}
		else {
			//update entity
			currentticker->function(currentticker);
		}
		currentticker = nextticker;
	}
}
