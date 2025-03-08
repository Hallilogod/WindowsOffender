/*Server status codes may NOT be bigger than 9999 aka 4 base10 digits*/

typedef unsigned long SERVER_STATUS_CODE;

#define SERVER_STATUS_SUCCESS                       (0UL)
#define SERVER_STATUS_INVALID_ARGUMENTS             (1UL)
#define SERVER_STATUS_REQUEST_HANDLING_ERROR        (2UL)
#define SERVER_STATUS_DRIVER_IOCTL_ERROR            (3UL)
#define SERVER_STATUS_UNKNOWN_REQUEST               (4UL)
#define SERVER_STATUS_NOT_ENOUGH_ARGUMENTS          (5UL)
#define SERVER_STATUS_INVALID_REQUEST_SYNTAX        (6UL)
#define SERVER_STATUS_THREAD_CREATION_FAILED        (7UL)

#define SERVER_STATUS_THREADED_ACTION               (50UL)
#define SERVER_STATUS_UNSUCCESSFUL                  (100UL)

#define SERVER_SUCCESS(StatusCode) (((SERVER_STATUS_CODE)(StatusCode)) == SERVER_STATUS_SUCCESS)