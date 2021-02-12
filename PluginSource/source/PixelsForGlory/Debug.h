#pragma once

#include <stdio.h>
#include <string>

#define PFG_EDITORLOG(msg) {\
    auto log_msg = "[Raytracing Plugin] " + std::string(msg) + "\n"; \
    fprintf(stdout, log_msg.c_str()); \
}

#define PFG_EDITORLOGW(msg) {\
    auto log_msg = L"[Raytracing Plugin] " + std::wstring(msg) + L"\n"; \
    fwprintf(stdout, log_msg.c_str()); \
}

#define PFG_EDITORLOGERROR(msg) {\
    auto log_msg = "[Raytracing Plugin] " + std::string(msg) + "\n"; \
    fprintf(stderr, log_msg.c_str()); \
}

#define PFG_EDITORLOGERRORW(msg) {\
    auto log_msg = L"[Raytracing Plugin] " + std::wstring(msg) + L"\n"; \
    fwprintf(stderr, log_msg.c_str()); \
}
