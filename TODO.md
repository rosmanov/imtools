# TODO

- No gain with threading. Probably, because most of the time is
consumed by `matchTemplate()` which is mutex-locked. Thus, all threads are still waiting for
each other (until `matchTemplate` finishes). One possible solution is to pre-fetch results of
`matchTemplate()` using threads.
