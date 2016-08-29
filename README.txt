a3search v1.0 written by C.Geng

Reference:
[1] GNU grep source code
[2] A. Hume, D. Sunday, Fast string searching, Softw. Pract. Exp. 21 (11) (1991) 1221–1248.

1. Core ideas about this program

In this program, I am required to implement multi-pattern searching among multi files. At first, I realise that there is a software called GNU Grep which has similar function to the requirments of this assignments on GNU/Linux platform. In GNU Grep, there are two core ideas about how to improve performance on pattern matching. The first is using a large buffer to read raw data and searching pattern by Boyer-Moore algorithm. The second is using a lookup table for skipping when no matching is detected. These ideas are all implemented in this assigment.

(1) Beacuse every file in the destination folder is no more than 10MB, due to the limited memory size(12MB),I set a threshold 8MB for every file. Appaerntly, this size is enough for most files in a folder. For file which is less than 8MB, I allocate appropriate buffer to read the file through Linux I/O api. For file which is larger than 8MB, I read 8MB data from the file first then search from the last to find the last "\n" as file split position.

(2) Instead of shifting function, a skip array is build for all ASCII symbols (256 symbols).

2. Pattern matching algorithm

As mentioned above, Boyer-Moore which is probabily the most fast pattern matching is used in this program. However, I use a varient call Boyer-Moore-Horspool-Sunday(BMHS) instead of the original version. In BMHS algorithm, it only use bad character algorithm. In the first, scan the text window in original text and pattern from left to right. When mismatching detected, get the character directly right of the text window and check whether it is in the pattern or not. If it is in the pattern, then shift the pattern and align at the original text. If not, shift the pattern and align the most left character at the next charator.
