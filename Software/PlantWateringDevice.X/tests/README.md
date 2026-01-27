# Purpose

This is a set of hardware tests intended to test peripherals connected to each of the PIC pins.

# Usage

They are not automated, meaning that for each test:
* the content of the test needs to be copied (it should replace the original content) to [main.s](../main.s)
* At the beginning of each test there is a comment with:
  * List of actions after the software was started
  * expected behaviour.
