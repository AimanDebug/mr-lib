#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "log.h"

int send_input_to_mapper(mr_log_file_t* log_file, int write_fd,
                         const char* input_path);
int receive_output_from_reducer(mr_log_file_t* log_file, int read_fd,
                                const char* output_path);

#endif /* PROTOCOL_H */
