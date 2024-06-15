#ifndef SUPPRESSION_HPP
#define SUPPRESSION_HPP

const char * const def_top_frame_suppressions[] = {
    ""
};

const char * const def_frame_suppressions[] = {
    "java.lang.invoke.*",
    "java.util.concurrent.locks.*"
};

class JTSanSuppression {
    public:
        static void init();
        
};


#endif
