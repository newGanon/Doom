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
	if (ticker->next && ticker->prev) {
		ticker->next->prev = ticker->prev;
		ticker->prev->next = ticker->next;
		ticker->function = (actionf)(-1);
	}
}

void run_tickers() {
	ticker* currentticker;
	currentticker = tickercap.next;
	while (currentticker != &tickercap) {
		ticker* nextticker = currentticker->next;
		if (currentticker->function == (actionf)(-1)) {
			remove_ticker(currentticker);
		}
		else {
			//update entity
			currentticker->function(currentticker);
		}
		currentticker = nextticker;
	}
}
