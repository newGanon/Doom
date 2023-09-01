#include "ticker.h"
#include "util.h"
//head and tail of tickerlist

ticker tickercap;

void init_tickers() {
	tickercap.prev = tickercap.next = &tickercap;
}

void add_ticker(ticker* ticker) {
	tickercap.prev->next = ticker;
	ticker->next = &tickercap;
	ticker->prev = tickercap.prev;
	tickercap.prev = ticker;
}

void remove_ticker(ticker* ticker) {
	if (ticker == &tickercap) return;
	ticker->next->prev = ticker->prev;
	ticker->prev->next = ticker->next;
}

void run_tickers() {
	ticker* currentticker;
	currentticker = tickercap.next;
	while (currentticker != &tickercap) {
		if (currentticker->function == (actionf)(-1)) {
			currentticker->next->prev = currentticker->prev;
			currentticker->prev->next = currentticker->next;
		}
		else {
			currentticker->function(currentticker);
		}
		currentticker = currentticker->next;
	}
}
