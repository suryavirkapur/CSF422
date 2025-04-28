# Assignment 4

## Suryavir Kapur

## 2022A7PS0293U

## Note

- I ignored the BGL.txt which is raw log file I found on the paper linked to the GitHub. Instead use the structured log file with around 2000 entries.
- I have sorted the events by their EventID, and ignored events that did not follow the pattern E{0-9}+.

## TODO

Consider a log dataset given at github. This log file contains record of various events. Write
MPI-OpenMP hybrid program for the following requirements.:
(a) Data in file should be read by each process parallely. Each process reads its data using
MPI-IO. MPI_File_open and MPI_File_read_at may be used.
(b) File is distributed evenly among all processes. A line can’t be split across processes.
(c) Goal is to find the count of each event and print top 10 occuring events along with
their counts.
(d) All events and their count is printed to file event_count.txt. Top 10 events are printed
to to_10.txt. Writing can use MPI-IO for parallel IO.

Files Expected: A tar file ID_the4.tar containing mpi.c, makefile, screenshots. Screenshots
should have idno of the student clearly visible in the terminal.
