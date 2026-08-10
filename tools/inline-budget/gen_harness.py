import sys
S = int(sys.argv[1])       # statements in the callee
N = int(sys.argv[2])       # call sites in the caller
PAD = int(sys.argv[3])     # padding statements in the caller, before the sites
body = "\n".join("    gA[%d] = gA[%d] + row;" % (i, i+1) for i in range(S))
pad  = "\n".join("    gB[%d] = gB[%d] + p;" % (i % 60, (i+1) % 60) for i in range(PAD))
sites= "\n".join("    leaf(%d);" % i for i in range(N))
print("""int gA[256];
int gB[256];

inline void leaf(int row) {
%s
}

void callerX(int p) {
%s
%s
}
""" % (body, pad, sites))
