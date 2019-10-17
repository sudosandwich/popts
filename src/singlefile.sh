sed -e '/#[[:space:]]*include "opt.h"/{r opt.h' -e 'd}' opts.h > singleheader.h
sed -i -e '/#[[:space:]]*include "opt.inl.h"/{r opt.inl.h' -e 'd}' singleheader.h
sed -i -e '/#[[:space:]]*include "opts.inl.h"/{r opts.inl.h' -e 'd}' singleheader.h
sed -i -e '/#[[:space:]]*include "typedefs.h"/{r typedefs.h' -e 'd}' singleheader.h
clang-format -i -style file -fallback-style llvm singleheader.h
mv singleheader.h ../include/popts.hpp
