#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

typedef struct {
    char *event;
    long long count;
} EventCount;

typedef struct {
    EventCount *data;
    size_t size;
    size_t capacity;
} EventCountArray;

void initEventCountArray(EventCountArray *arr, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 10;
    arr->data = malloc(initial_capacity * sizeof(EventCount));
    if (!arr->data) {
        perror("malloc EventCountArray failed");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    arr->size = 0;
    arr->capacity = initial_capacity;
}

int addOrUpdateEvent(EventCountArray *arr, const char *event_key, long long count_to_add) {
    if (event_key == NULL) return 0;
    for (size_t i = 0; i < arr->size; ++i) {
        if (strcmp(arr->data[i].event, event_key) == 0) {
            arr->data[i].count += count_to_add;
            return 1;
        }
    }
    if (arr->size >= arr->capacity) {
        size_t new_capacity = arr->capacity + arr->capacity/2;
        EventCount *temp = realloc(arr->data, new_capacity * sizeof(EventCount));
        if (!temp) {
            perror("realloc EventCountArray failed");
            return 0;
        }
        arr->data = temp;
        arr->capacity = new_capacity;
    }
    arr->data[arr->size].event = strdup(event_key);
    if (!arr->data[arr->size].event) {
        perror("strdup failed");
        return 0;
    }
    arr->data[arr->size].count = count_to_add;
    arr->size++;
    return 1;
}

void freeEventCountArray(EventCountArray *arr) {
    if (arr) {
        for (size_t i = 0; i < arr->size; ++i) {
            free(arr->data[i].event);
        }
        free(arr->data);
        arr->data = NULL;
        arr->size = arr->capacity = 0;
    }
}

// check if key is of the regex format E{0-9}+
int validEventKey(const char *key) {
    if (!key) return 0;
    if (strlen(key) < 3 || strlen(key) > 5) return 0;
    if (key[0] != 'E') return 0;
    for (size_t i = 1; i < strlen(key); i++) {
        if (!isdigit((unsigned char)key[i])) return 0;
    }
    return 1;
}

// get event key as the field between the second-last and last commas
char* extract_event_key(const char *line) {
    size_t len = strlen(line);
    const char *ptr = line + len - 1;
    const char *last_comma = NULL, *second_last_comma = NULL;
    while (ptr >= line && isspace((unsigned char)*ptr)) ptr--;
    while (ptr >= line) {
        if (*ptr == ',') { last_comma = ptr; break; }
        ptr--;
    }
    if (!last_comma || last_comma == line) return NULL;
    ptr = last_comma - 1;
    while (ptr >= line) {
        if (*ptr == ',') { second_last_comma = ptr; break; }
        ptr--;
    }
    if (!second_last_comma) return NULL;
    const char *start = second_last_comma + 1;
    size_t key_len = last_comma - start;
    if (key_len == 0) return NULL;
    char *key = malloc(key_len + 1);
    if (!key) {
        perror("malloc key failed");
        return NULL;
    }
    strncpy(key, start, key_len);
    key[key_len] = '\0';
    return key;
}

int compareEventCounts(const void *a, const void *b) {
    EventCount *ea = (EventCount *)a;
    EventCount *eb = (EventCount *)b;
    if (eb->count > ea->count) return 1;
    if (eb->count < ea->count) return -1;
    return strcmp(ea->event, eb->event);
}

int main(int argc, char* argv[]) {
    printf("Suryavir Kapur (2022A7PS0293U)\n");
    MPI_Init(&argc, &argv);
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    const char *input_filename = "BGL.log";
    const char *count_filename = "event_count.txt";
    const char *top10_filename = "top_10.txt";
    double start_time = 0.0, end_time;
    if (rank == 0) start_time = MPI_Wtime();

    MPI_File fh;
    if (MPI_File_open(MPI_COMM_WORLD, input_filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh) != MPI_SUCCESS) {
        if (rank == 0) fprintf(stderr, "Error opening file %s\n", input_filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);
    if (file_size == 0) {
        if (rank == 0) {
            FILE *f = fopen(count_filename, "w");
            if (f) { fclose(f); }
            f = fopen(top10_filename, "w");
            if (f) { fprintf(f, "Top 10 Events\nNo events\n"); fclose(f); }
        }
        MPI_File_close(&fh);
        MPI_Finalize();
        return 0;
    }

    // figure out the offset for each chunk
    MPI_Offset chunk = file_size / nprocs;
    MPI_Offset start = rank * chunk;
    MPI_Offset end = (rank == nprocs - 1) ? file_size : (rank + 1) * chunk;
    if (rank > 0) {
        char c;
        MPI_File_read_at(fh, start, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE);
        while (c != '\n' && start < file_size) {
            start++;
            if (MPI_File_read_at(fh, start, &c, 1, MPI_CHAR, MPI_STATUS_IGNORE) != MPI_SUCCESS) break;
        }
        if (start < file_size) start++;  
    }
    MPI_Offset local_size = (end > start) ? end - start : 0;
    char *buffer = malloc(local_size + 1);
    if (!buffer) {
        perror("malloc buffer failed");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_File_read_at_all(fh, start, buffer, local_size, MPI_CHAR, MPI_STATUS_IGNORE);
    buffer[local_size] = '\0';
    MPI_File_close(&fh);

    EventCountArray local_counts;
    initEventCountArray(&local_counts, 50);

    // process the buffer line by line using OpenMP.
    // we create thread-local arrays and merge them.
    #pragma omp parallel
    {
        EventCountArray thread_counts;
        initEventCountArray(&thread_counts, 10);
        #pragma omp for schedule(dynamic)
        for (MPI_Offset i = 0; i < local_size; i++) {
            if (i == 0 || buffer[i-1] == '\n') {
                char *line = buffer + i;
                char *newline = strchr(line, '\n');
                if (!newline) newline = line + strlen(line);
                size_t len = newline - line;
                if (len == 0) continue;
                char *line_copy = strndup(line, len);
                if (!line_copy) continue;
                char *event_key = extract_event_key(line_copy);
                free(line_copy);
                if (event_key && validEventKey(event_key)) {
                    addOrUpdateEvent(&thread_counts, event_key, 1);
                }
                free(event_key);
            }
        }
        #pragma omp critical
        {
            for (size_t j = 0; j < thread_counts.size; j++) {
                addOrUpdateEvent(&local_counts, thread_counts.data[j].event, thread_counts.data[j].count);
            }
        }
        freeEventCountArray(&thread_counts);
    }
    free(buffer);
    int n_events = (int)local_counts.size;
    if (rank == 0) {
        EventCountArray global_counts;
        initEventCountArray(&global_counts, local_counts.size + 10);
        for (size_t i = 0; i < local_counts.size; i++) {
            addOrUpdateEvent(&global_counts, local_counts.data[i].event, local_counts.data[i].count);
        }
        for (int src = 1; src < nprocs; src++) {
            int remote_n;
            MPI_Recv(&remote_n, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < remote_n; i++) {
                int str_len;
                MPI_Recv(&str_len, 1, MPI_INT, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                char *event_str = malloc(str_len + 1);
                MPI_Recv(event_str, str_len, MPI_CHAR, src, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                event_str[str_len] = '\0';
                long long cnt;
                MPI_Recv(&cnt, 1, MPI_LONG_LONG, src, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                addOrUpdateEvent(&global_counts, event_str, cnt);
                free(event_str);
            }
        }
        qsort(global_counts.data, global_counts.size, sizeof(EventCount), compareEventCounts);
        FILE *fout = fopen(count_filename, "w");
        if (fout) {
            for (size_t i = 0; i < global_counts.size; i++) {
                fprintf(fout, "%s: %lld\n", global_counts.data[i].event, global_counts.data[i].count);
            }
            fclose(fout);
        }
        fout = fopen(top10_filename, "w");
        if (fout) {
            fprintf(fout, "Top 10 Events\n");
            size_t top_n = (global_counts.size < 10) ? global_counts.size : 10;
            for (size_t i = 0; i < top_n; i++) {
                fprintf(fout, "%s: %lld\n", global_counts.data[i].event, global_counts.data[i].count);
            }
            fclose(fout);
        }
        freeEventCountArray(&global_counts);
    } else {
        MPI_Send(&n_events, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (size_t i = 0; i < local_counts.size; i++) {
            int len = (int)strlen(local_counts.data[i].event);
            MPI_Send(&len, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(local_counts.data[i].event, len, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
            MPI_Send(&local_counts.data[i].count, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD);
        }
    }
    freeEventCountArray(&local_counts);
    if (rank == 0) {
        end_time = MPI_Wtime();
        printf("Total execution time: %f seconds\n", end_time - start_time);
    }
    MPI_Finalize();
    return 0;
}
