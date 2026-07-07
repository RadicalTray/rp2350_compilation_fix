Import("env")

env.Append(CFLAGS=[
    # '#define lwip_itoa t1s_lwip_itoa' macro "declares" the function 't1s_lwip_itoa' somehow
    # even though other functions aren't like this (if i understood the error correctly)
    "-Wno-implicit-function-declaration", # Completely ignore the warning
    # "-Wno-error=implicit-function-declaration", # Keep the warning but don't error
])
