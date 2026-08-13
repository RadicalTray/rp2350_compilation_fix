Import("env")

# Both C and C++
env.Append(CCFLAGS=[
    "-DLWIP_HAVE_LOOPIF=1", # enable loopback
])

# C
env.Append(CFLAGS=[
    # '#define lwip_itoa t1s_lwip_itoa' macro "declares" the function 't1s_lwip_itoa' somehow
    # even though other functions aren't like this (if i understood the error correctly)
    "-Wno-implicit-function-declaration", # Completely ignore the warning
    # "-Wno-error=implicit-function-declaration", # Keep the warning but don't error
])

# C++
env.Append(CXXFLAGS=[
])
