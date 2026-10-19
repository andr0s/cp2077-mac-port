// cpre: tiny static cross-reference tool for the macOS Cyberpunk2077 arm64 Mach-O.
// Reads the binary only; never executes it.
//
// Usage: cpre <binary> <command> [args...]   (several commands may be chained with ';' as separate args)
//   info
//   func <addr>                 containing function [start,end) from LC_FUNCTION_STARTS (+symbol)
//   callers <addr>              BL/B sites whose target == addr
//   callees <addr>              BL/B targets inside the function containing addr
//   xref <addr>                 code refs (ADR/ADRP+ADD/LDR/STR) and data pointers to exactly addr,
//                               plus refs through GOT slots bound to a symbol defined at addr
//   xrefr <lo> <hi>             code refs and data pointers into [lo,hi)
//   str <substring>             C strings containing substring, with referencing functions
//   strx <substring>            exact C string match only
//   strrefs <addr>              C strings referenced by the function containing addr
//   datarefs <addr>             all non-string addresses referenced by the function containing addr
//   ptrsto <addr>               data locations (rebases) whose value == addr
//   ptrsat <addr> <n>           decode n pointer slots starting at data address addr
//   gotname <regex>             GOT/data binds whose symbol matches regex, with code refs to the slot
//   sym <regex>                 defined symbols matching regex (mangled)
//   symat <addr>                symbol(s) at or containing addr
//   imm <value>                 MOVZ/MOVK-materialized 32-bit constants == value (hex or dec)
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <map>
#include <regex>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace
{
struct Section
{
    std::string seg, name;
    uint64_t addr, size;
    uint32_t offset;
};
struct Segment
{
    std::string name;
    uint64_t vmaddr, vmsize, fileoff, filesize;
};
struct Sym
{
    uint64_t addr;
    std::string name;
};
struct Ref
{
    uint64_t src, dst;
    uint8_t kind; // 0 adr, 1 adrp+add, 2 adrp+ldr, 3 adrp+str, 4 adrp-page-only, 5 base+off via derived reg
};
struct Call
{
    uint64_t src, dst;
    uint8_t kind; // 0 BL, 1 B
};
struct Ptr
{
    uint64_t loc, dst;
};
struct Bind
{
    uint64_t loc;
    uint32_t ordinal;
};
struct Imm
{
    uint64_t src;
    uint32_t value;
};

const uint8_t* g_file = nullptr;
size_t g_size = 0;
std::vector<Section> g_sects;
std::vector<Segment> g_segs;
std::vector<uint64_t> g_funcs;
std::vector<Sym> g_syms; // sorted by addr
std::vector<Ref> g_refs; // sorted by dst
std::vector<Call> g_calls; // sorted by dst
std::vector<Ptr> g_ptrs; // sorted by dst
std::vector<Bind> g_binds; // sorted by loc
std::vector<std::string> g_importNames;
std::vector<Imm> g_imms;
uint64_t g_textBase = 0x100000000ull;
const Section* g_text = nullptr;
const Section* g_cstring = nullptr;

uint64_t ParseNum(const char* s)
{
    return strtoull(s, nullptr, 0);
}

const uint8_t* VmToFile(uint64_t vm)
{
    for (auto& s : g_segs)
    {
        if (vm >= s.vmaddr && vm < s.vmaddr + s.filesize)
            return g_file + s.fileoff + (vm - s.vmaddr);
    }
    return nullptr;
}

const Section* SectFor(uint64_t vm)
{
    for (auto& s : g_sects)
        if (vm >= s.addr && vm < s.addr + s.size)
            return &s;
    return nullptr;
}

uint64_t ReadUleb(const uint8_t*& p, const uint8_t* end)
{
    uint64_t r = 0;
    int shift = 0;
    while (p < end)
    {
        uint8_t b = *p++;
        r |= uint64_t(b & 0x7f) << shift;
        if (!(b & 0x80))
            break;
        shift += 7;
    }
    return r;
}

int64_t Sext(uint64_t v, int bits)
{
    uint64_t m = 1ull << (bits - 1);
    return int64_t((v ^ m) - m);
}

void Load(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        exit(1);
    }
    struct stat st;
    fstat(fd, &st);
    g_size = st.st_size;
    g_file = static_cast<const uint8_t*>(mmap(nullptr, g_size, PROT_READ, MAP_PRIVATE, fd, 0));
    close(fd);

    uint32_t ncmds;
    memcpy(&ncmds, g_file + 16, 4);
    size_t off = 32;
    uint32_t fsOff = 0, fsSize = 0, symOff = 0, nsyms = 0, strOff = 0, strSize = 0, cfOff = 0, cfSize = 0;
    for (uint32_t i = 0; i < ncmds; i++)
    {
        uint32_t cmd, cmdsize;
        memcpy(&cmd, g_file + off, 4);
        memcpy(&cmdsize, g_file + off + 4, 4);
        if (cmd == 0x19)
        {
            Segment seg;
            seg.name.assign(reinterpret_cast<const char*>(g_file + off + 8), strnlen((const char*)g_file + off + 8, 16));
            memcpy(&seg.vmaddr, g_file + off + 24, 8);
            memcpy(&seg.vmsize, g_file + off + 32, 8);
            memcpy(&seg.fileoff, g_file + off + 40, 8);
            memcpy(&seg.filesize, g_file + off + 48, 8);
            g_segs.push_back(seg);
            uint32_t nsects;
            memcpy(&nsects, g_file + off + 64, 4);
            for (uint32_t s = 0; s < nsects; s++)
            {
                const uint8_t* sp = g_file + off + 72 + s * 80;
                Section sec;
                sec.name.assign((const char*)sp, strnlen((const char*)sp, 16));
                sec.seg.assign((const char*)sp + 16, strnlen((const char*)sp + 16, 16));
                memcpy(&sec.addr, sp + 32, 8);
                memcpy(&sec.size, sp + 40, 8);
                memcpy(&sec.offset, sp + 48, 4);
                g_sects.push_back(sec);
            }
        }
        else if (cmd == 0x26)
        {
            memcpy(&fsOff, g_file + off + 8, 4);
            memcpy(&fsSize, g_file + off + 12, 4);
        }
        else if (cmd == 0x2)
        {
            memcpy(&symOff, g_file + off + 8, 4);
            memcpy(&nsyms, g_file + off + 12, 4);
            memcpy(&strOff, g_file + off + 16, 4);
            memcpy(&strSize, g_file + off + 20, 4);
        }
        else if (cmd == 0x80000034)
        {
            memcpy(&cfOff, g_file + off + 8, 4);
            memcpy(&cfSize, g_file + off + 12, 4);
        }
        off += cmdsize;
    }
    for (auto& s : g_sects)
    {
        if (s.seg == "__TEXT" && s.name == "__text")
            g_text = &s;
        if (s.seg == "__TEXT" && s.name == "__cstring")
            g_cstring = &s;
    }
    for (auto& s : g_segs)
        if (s.name == "__TEXT")
            g_textBase = s.vmaddr;

    // Function starts
    {
        const uint8_t* p = g_file + fsOff;
        const uint8_t* e = p + fsSize;
        uint64_t a = g_textBase;
        while (p < e)
        {
            uint64_t d = ReadUleb(p, e);
            if (!d)
                break;
            a += d;
            g_funcs.push_back(a);
        }
    }
    // Symbols
    for (uint32_t i = 0; i < nsyms; i++)
    {
        const uint8_t* np = g_file + symOff + i * 16;
        uint32_t strx;
        uint8_t type;
        uint64_t value;
        memcpy(&strx, np, 4);
        type = np[4];
        memcpy(&value, np + 8, 8);
        if ((type & 0xe0) != 0)
            continue; // stab
        if ((type & 0x0e) != 0x0e)
            continue; // not N_SECT
        if (strx >= strSize)
            continue;
        g_syms.push_back({value, std::string((const char*)g_file + strOff + strx)});
    }
    std::sort(g_syms.begin(), g_syms.end(), [](auto& a, auto& b) { return a.addr < b.addr; });

    // Chained fixups
    if (cfOff)
    {
        const uint8_t* h = g_file + cfOff;
        uint32_t startsOff, importsOff, symbolsOff, importsCount, importsFormat;
        memcpy(&startsOff, h + 4, 4);
        memcpy(&importsOff, h + 8, 4);
        memcpy(&symbolsOff, h + 12, 4);
        memcpy(&importsCount, h + 16, 4);
        memcpy(&importsFormat, h + 20, 4);
        for (uint32_t i = 0; i < importsCount; i++)
        {
            uint32_t nameOff = 0;
            if (importsFormat == 1)
            {
                uint32_t v;
                memcpy(&v, h + importsOff + i * 4, 4);
                nameOff = v >> 9;
            }
            else if (importsFormat == 2)
            {
                uint32_t v;
                memcpy(&v, h + importsOff + i * 8, 4);
                nameOff = v >> 9;
            }
            else
            {
                uint64_t v;
                memcpy(&v, h + importsOff + i * 16, 8);
                nameOff = uint32_t(v >> 32);
            }
            g_importNames.emplace_back((const char*)h + symbolsOff + nameOff);
        }
        const uint8_t* img = h + startsOff;
        uint32_t segCount;
        memcpy(&segCount, img, 4);
        for (uint32_t si = 0; si < segCount && si < g_segs.size(); si++)
        {
            uint32_t so;
            memcpy(&so, img + 4 + si * 4, 4);
            if (!so)
                continue;
            const uint8_t* ss = img + so;
            uint16_t pageSize, fmt, pageCount;
            uint64_t segOffset;
            memcpy(&pageSize, ss + 4, 2);
            memcpy(&fmt, ss + 6, 2);
            memcpy(&segOffset, ss + 8, 8);
            memcpy(&pageCount, ss + 20, 2);
            const Segment& seg = g_segs[si];
            for (uint32_t pg = 0; pg < pageCount; pg++)
            {
                uint16_t start;
                memcpy(&start, ss + 22 + pg * 2, 2);
                if (start == 0xFFFF)
                    continue;
                uint64_t locOff = uint64_t(pg) * pageSize + start;
                while (true)
                {
                    uint64_t vm = seg.vmaddr + locOff;
                    const uint8_t* fp = g_file + seg.fileoff + locOff;
                    if (locOff >= seg.filesize)
                        break;
                    uint64_t raw;
                    memcpy(&raw, fp, 8);
                    bool bind = raw >> 63;
                    uint64_t next = (raw >> 51) & 0xFFF;
                    if (bind)
                    {
                        g_binds.push_back({vm, uint32_t(raw & 0xFFFFFF)});
                    }
                    else
                    {
                        uint64_t target = raw & 0xFFFFFFFFFull;
                        uint64_t high8 = (raw >> 36) & 0xFF;
                        uint64_t dst = (fmt == 6 ? g_textBase + target : target) | (high8 << 56);
                        g_ptrs.push_back({vm, dst});
                    }
                    if (!next)
                        break;
                    locOff += next * 4;
                }
            }
        }
    }
    std::sort(g_ptrs.begin(), g_ptrs.end(), [](auto& a, auto& b) { return a.dst < b.dst || (a.dst == b.dst && a.loc < b.loc); });
    std::sort(g_binds.begin(), g_binds.end(), [](auto& a, auto& b) { return a.loc < b.loc; });

    // Code scan
    struct RegState
    {
        uint64_t val;
        bool valid;
        bool page; // value is an ADRP page (not yet completed)
    };
    RegState regs[32];
    auto clearAll = [&]() {
        for (auto& r : regs)
            r.valid = false;
    };
    clearAll();
    const uint8_t* code = g_file + g_text->offset;
    uint64_t n = g_text->size / 4;
    size_t fi = std::lower_bound(g_funcs.begin(), g_funcs.end(), g_text->addr) - g_funcs.begin();
    uint32_t movzVal[32];
    bool movzValid[32] = {};
    for (uint64_t i = 0; i < n; i++)
    {
        uint64_t pc = g_text->addr + i * 4;
        while (fi < g_funcs.size() && g_funcs[fi] < pc)
            fi++;
        if (fi < g_funcs.size() && g_funcs[fi] == pc)
        {
            clearAll();
            memset(movzValid, 0, sizeof(movzValid));
        }
        uint32_t w;
        memcpy(&w, code + i * 4, 4);
        uint32_t rd = w & 31, rn = (w >> 5) & 31;
        if ((w & 0x7C000000) == 0x14000000) // B / BL
        {
            int64_t imm = Sext(w & 0x3FFFFFF, 26) * 4;
            g_calls.push_back({pc, uint64_t(int64_t(pc) + imm), uint8_t((w >> 31) ? 0 : 1)});
            if (w >> 31)
            {
                for (int r = 0; r <= 18; r++)
                    regs[r].valid = false;
                regs[30].valid = false;
            }
            else
                clearAll();
            continue;
        }
        if ((w & 0x9F000000) == 0x90000000) // ADRP
        {
            uint64_t immlo = (w >> 29) & 3, immhi = (w >> 5) & 0x7FFFF;
            int64_t imm = Sext((immhi << 2) | immlo, 21) << 12;
            regs[rd] = {uint64_t(int64_t(pc & ~0xFFFull) + imm), true, true};
            continue;
        }
        if ((w & 0x9F000000) == 0x10000000) // ADR
        {
            uint64_t immlo = (w >> 29) & 3, immhi = (w >> 5) & 0x7FFFF;
            int64_t imm = Sext((immhi << 2) | immlo, 21);
            uint64_t t = uint64_t(int64_t(pc) + imm);
            g_refs.push_back({pc, t, 0});
            regs[rd] = {t, true, false};
            continue;
        }
        if ((w & 0xFF000000) == 0x91000000) // ADD x, x, #imm
        {
            uint64_t imm = (w >> 10) & 0xFFF;
            if ((w >> 22) & 1)
                imm <<= 12;
            if (regs[rn].valid)
            {
                uint64_t t = regs[rn].val + imm;
                g_refs.push_back({pc, t, uint8_t(regs[rn].page ? 1 : 5)});
                regs[rd] = {t, true, false};
            }
            else
                regs[rd].valid = false;
            continue;
        }
        // MOVZ/MOVK 32-bit constants (w or x)
        if ((w & 0x7F800000) == 0x52800000)
        {
            uint32_t hw = (w >> 21) & 3, imm = (w >> 5) & 0xFFFF;
            if (hw == 0)
            {
                movzVal[rd] = imm;
                movzValid[rd] = true;
                g_imms.push_back({pc, imm});
            }
            else
                movzValid[rd] = false;
            regs[rd].valid = false;
            continue;
        }
        if ((w & 0x7F800000) == 0x72800000)
        {
            uint32_t hw = (w >> 21) & 3, imm = (w >> 5) & 0xFFFF;
            if (hw == 1 && movzValid[rd])
            {
                g_imms.push_back({pc, movzVal[rd] | (imm << 16)});
            }
            movzValid[rd] = false;
            continue;
        }
        if ((w & 0x7F800000) == 0x12800000) // MOVN
        {
            uint32_t hw = (w >> 21) & 3, imm = (w >> 5) & 0xFFFF;
            if (hw == 0)
                g_imms.push_back({pc, ~imm});
            regs[rd].valid = false;
            continue;
        }
        // Loads/stores unsigned immediate (size bits 31:30, V bit 26, opc 23:22)
        if ((w & 0x3B000000) == 0x39000000)
        {
            uint32_t size = w >> 30, v = (w >> 26) & 1, opc = (w >> 22) & 3;
            uint64_t imm12 = (w >> 10) & 0xFFF;
            uint32_t scale = size;
            if (v && opc >= 2)
                scale = 4; // 128-bit q
            uint64_t off = imm12 << scale;
            bool isLoad = opc != 0;
            if (regs[rn].valid)
            {
                uint64_t t = regs[rn].val + off;
                g_refs.push_back({pc, t, uint8_t(regs[rn].page ? (isLoad ? 2 : 3) : 5)});
            }
            if (isLoad && !v)
                regs[rd].valid = false;
            continue;
        }
        // Generic: other loads/stores
        if ((w & 0x0A000000) == 0x08000000)
        {
            bool isLoad = (w >> 22) & 1;
            bool v = (w >> 26) & 1;
            if (isLoad && !v)
            {
                regs[rd].valid = false;
                // pair loads
                if ((w & 0x3A000000) == 0x28000000)
                    regs[(w >> 10) & 31].valid = false;
            }
            // writeback forms modify rn; be conservative
            if ((w & 0x3A000000) == 0x28000000 || (w & 0x3B200C00) == 0x38000400 || (w & 0x3B200C00) == 0x38000C00)
                regs[rn].valid = false;
            continue;
        }
        // RET / BR / BLR
        if ((w & 0xFFFFFC1F) == 0xD65F0000 || (w & 0xFFFFFC1F) == 0xD61F0000)
        {
            clearAll();
            continue;
        }
        if ((w & 0xFFFFFC1F) == 0xD63F0000)
        {
            for (int r = 0; r <= 18; r++)
                regs[r].valid = false;
            regs[30].valid = false;
            continue;
        }
        // MOV (register) = ORR xd, xzr, xm : propagate
        if ((w & 0xFFE0FFE0) == 0xAA0003E0)
        {
            uint32_t rm = (w >> 16) & 31;
            regs[rd] = regs[rm];
            continue;
        }
        // default: assume writes rd
        regs[rd].valid = false;
        movzValid[rd] = false;
    }
    std::sort(g_refs.begin(), g_refs.end(), [](auto& a, auto& b) { return a.dst < b.dst || (a.dst == b.dst && a.src < b.src); });
    std::sort(g_calls.begin(), g_calls.end(), [](auto& a, auto& b) { return a.dst < b.dst || (a.dst == b.dst && a.src < b.src); });
    std::sort(g_imms.begin(), g_imms.end(), [](auto& a, auto& b) { return a.value < b.value || (a.value == b.value && a.src < b.src); });
}

std::pair<uint64_t, uint64_t> FuncOf(uint64_t a)
{
    auto it = std::upper_bound(g_funcs.begin(), g_funcs.end(), a);
    if (it == g_funcs.begin())
        return {0, 0};
    uint64_t start = *(it - 1);
    uint64_t end = it == g_funcs.end() ? g_text->addr + g_text->size : *it;
    return {start, end};
}

const Sym* SymAt(uint64_t a)
{
    auto it = std::upper_bound(g_syms.begin(), g_syms.end(), a, [](uint64_t v, const Sym& s) { return v < s.addr; });
    if (it == g_syms.begin())
        return nullptr;
    return &*(it - 1);
}

std::string StubName(uint64_t a)
{
    for (auto& sec : g_sects)
    {
        if (sec.name != "__stubs" || a < sec.addr || a >= sec.addr + sec.size)
            continue;
        const uint8_t* p = VmToFile(a);
        if (!p)
            return "";
        uint32_t w0, w1;
        memcpy(&w0, p, 4);
        memcpy(&w1, p + 4, 4);
        if ((w0 & 0x9F000000) != 0x90000000 || (w1 & 0xFFC00000) != 0xF9400000)
            return "";
        uint64_t immlo = (w0 >> 29) & 3, immhi = (w0 >> 5) & 0x7FFFF;
        int64_t imm = Sext((immhi << 2) | immlo, 21) << 12;
        uint64_t slot = uint64_t(int64_t(a & ~0xFFFull) + imm) + (((w1 >> 10) & 0xFFF) << 3);
        auto it = std::lower_bound(g_binds.begin(), g_binds.end(), slot, [](const Bind& b, uint64_t v) { return b.loc < v; });
        if (it != g_binds.end() && it->loc == slot && it->ordinal < g_importNames.size())
            return "stub:" + g_importNames[it->ordinal];
        return "stub:?";
    }
    return "";
}

std::string FuncName(uint64_t a)
{
    if (auto sn = StubName(a); !sn.empty())
        return sn;
    auto [s, e] = FuncOf(a);
    std::string r;
    char buf[64];
    snprintf(buf, sizeof buf, "fn_%" PRIx64, s);
    r = buf;
    for (auto it = std::lower_bound(g_syms.begin(), g_syms.end(), s, [](const Sym& x, uint64_t v) { return x.addr < v; });
         it != g_syms.end() && it->addr == s; ++it)
    {
        r += " <" + it->name + ">";
        break;
    }
    return r;
}

std::string CStr(uint64_t a, size_t maxLen = 200)
{
    const uint8_t* p = VmToFile(a);
    if (!p)
        return "";
    std::string s;
    for (size_t i = 0; i < maxLen && p + i < g_file + g_size && p[i]; i++)
    {
        char c = p[i];
        if (c == '\n')
            s += "\\n";
        else if (c == '\t')
            s += "\\t";
        else if ((unsigned char)c < 0x20)
            s += '?';
        else
            s += c;
    }
    return s;
}

bool InCstring(uint64_t a)
{
    for (auto& s : g_sects)
        if ((s.name == "__cstring" || s.name == "__objc_methname" || s.name == "__objc_classname") && a >= s.addr &&
            a < s.addr + s.size)
            return true;
    return false;
}

const char* RefKind(uint8_t k)
{
    static const char* names[] = {"adr", "adrp+add", "adrp+ldr", "adrp+str", "page", "base+off"};
    return k < 6 ? names[k] : "?";
}

void PrintCodeRefs(uint64_t lo, uint64_t hi, const char* indent = "  ")
{
    auto it = std::lower_bound(g_refs.begin(), g_refs.end(), lo, [](const Ref& r, uint64_t v) { return r.dst < v; });
    for (; it != g_refs.end() && it->dst < hi; ++it)
        printf("%scode  %#" PRIx64 " -> %#" PRIx64 " (%s) in %s\n", indent, it->src, it->dst, RefKind(it->kind),
               FuncName(it->src).c_str());
}

void PrintPtrs(uint64_t lo, uint64_t hi, const char* indent = "  ")
{
    auto it = std::lower_bound(g_ptrs.begin(), g_ptrs.end(), lo, [](const Ptr& r, uint64_t v) { return r.dst < v; });
    for (; it != g_ptrs.end() && it->dst < hi; ++it)
    {
        const Section* s = SectFor(it->loc);
        const Sym* sym = SymAt(it->loc);
        printf("%sdata  %#" PRIx64 " [%s,%s] -> %#" PRIx64 "%s%s\n", indent, it->loc, s ? s->seg.c_str() : "?",
               s ? s->name.c_str() : "?", it->dst, sym ? "  near " : "", sym ? sym->name.c_str() : "");
    }
}

void PrintGotRefsForSymbolsAt(uint64_t a)
{
    // symbols defined at a
    for (auto it = std::lower_bound(g_syms.begin(), g_syms.end(), a, [](const Sym& x, uint64_t v) { return x.addr < v; });
         it != g_syms.end() && it->addr == a; ++it)
    {
        for (auto& b : g_binds)
        {
            if (b.ordinal < g_importNames.size() && g_importNames[b.ordinal] == it->name)
            {
                printf("  bind %#" PRIx64 " = %s\n", b.loc, it->name.c_str());
                PrintCodeRefs(b.loc, b.loc + 1, "    ");
            }
        }
    }
}

int Run(int argc, char** argv)
{
    std::string cmd = argv[0];
    if (cmd == "info")
    {
        printf("funcs %zu syms %zu refs %zu calls %zu ptrs %zu binds %zu imports %zu imms %zu\n", g_funcs.size(), g_syms.size(),
               g_refs.size(), g_calls.size(), g_ptrs.size(), g_binds.size(), g_importNames.size(), g_imms.size());
        for (auto& s : g_sects)
            printf("  %-14s %-18s %#" PRIx64 " %#" PRIx64 "\n", s.seg.c_str(), s.name.c_str(), s.addr, s.size);
        return 1;
    }
    if (cmd == "func" && argc >= 2)
    {
        uint64_t a = ParseNum(argv[1]);
        auto [s, e] = FuncOf(a);
        printf("%#" PRIx64 ": function %#" PRIx64 "..%#" PRIx64 " (size %#" PRIx64 ") %s\n", a, s, e, e - s, FuncName(a).c_str());
        return 2;
    }
    if (cmd == "callers" && argc >= 2)
    {
        uint64_t a = ParseNum(argv[1]);
        auto it = std::lower_bound(g_calls.begin(), g_calls.end(), a, [](const Call& c, uint64_t v) { return c.dst < v; });
        int cnt = 0;
        for (; it != g_calls.end() && it->dst == a; ++it, ++cnt)
            if (cnt < 400)
                printf("  %s %#" PRIx64 " in %s\n", it->kind ? "B " : "BL", it->src, FuncName(it->src).c_str());
        printf("callers of %#" PRIx64 ": %d\n", a, cnt);
        PrintPtrs(a, a + 1);
        return 2;
    }
    if (cmd == "callees" && argc >= 2)
    {
        auto [s, e] = FuncOf(ParseNum(argv[1]));
        std::vector<Call> cs;
        for (auto& c : g_calls)
            if (c.src >= s && c.src < e)
                cs.push_back(c);
        std::sort(cs.begin(), cs.end(), [](auto& a, auto& b) { return a.src < b.src; });
        for (auto& c : cs)
            printf("  %#" PRIx64 " %s %#" PRIx64 " %s\n", c.src, c.kind ? "B " : "BL", c.dst, FuncName(c.dst).c_str());
        return 2;
    }
    if (cmd == "xref" && argc >= 2)
    {
        uint64_t a = ParseNum(argv[1]);
        printf("xrefs to %#" PRIx64 ":\n", a);
        PrintCodeRefs(a, a + 1);
        PrintPtrs(a, a + 1);
        PrintGotRefsForSymbolsAt(a);
        return 2;
    }
    if (cmd == "xrefr" && argc >= 3)
    {
        uint64_t lo = ParseNum(argv[1]), hi = ParseNum(argv[2]);
        PrintCodeRefs(lo, hi);
        PrintPtrs(lo, hi);
        return 3;
    }
    if ((cmd == "str" || cmd == "strx") && argc >= 2)
    {
        std::string needle = argv[1];
        bool exact = cmd == "strx";
        for (auto& sec : g_sects)
        {
            if (sec.name != "__cstring")
                continue;
            const char* p = (const char*)g_file + sec.offset;
            const char* end = p + sec.size;
            int shown = 0;
            while (p < end && shown < 200)
            {
                size_t len = strnlen(p, end - p);
                std::string_view sv(p, len);
                bool hit = exact ? sv == needle : sv.find(needle) != std::string_view::npos;
                if (hit)
                {
                    uint64_t a = sec.addr + (p - ((const char*)g_file + sec.offset));
                    printf("%#" PRIx64 " \"%s\"\n", a, CStr(a, 160).c_str());
                    PrintCodeRefs(a, a + 1, "    ");
                    PrintPtrs(a, a + 1, "    ");
                    shown++;
                }
                p += len + 1;
            }
        }
        return 2;
    }
    if ((cmd == "strrefs" || cmd == "datarefs") && argc >= 2)
    {
        auto [s, e] = FuncOf(ParseNum(argv[1]));
        std::vector<Ref> rs;
        for (auto& r : g_refs)
            if (r.src >= s && r.src < e)
                rs.push_back(r);
        std::sort(rs.begin(), rs.end(), [](auto& a, auto& b) { return a.src < b.src; });
        for (auto& r : rs)
        {
            bool isStr = InCstring(r.dst);
            if ((cmd == "strrefs") != isStr)
                continue;
            if (isStr)
                printf("  %#" PRIx64 " -> %#" PRIx64 " \"%s\"\n", r.src, r.dst, CStr(r.dst, 160).c_str());
            else
            {
                const Section* sec = SectFor(r.dst);
                const Sym* sym = SymAt(r.dst);
                std::string bindName;
                for (auto it = std::lower_bound(g_binds.begin(), g_binds.end(), r.dst,
                                                [](const Bind& b, uint64_t v) { return b.loc < v; });
                     it != g_binds.end() && it->loc == r.dst; ++it)
                    if (it->ordinal < g_importNames.size())
                        bindName = g_importNames[it->ordinal];
                printf("  %#" PRIx64 " -> %#" PRIx64 " (%s) [%s,%s]%s%s%s%s\n", r.src, r.dst, RefKind(r.kind),
                       sec ? sec->seg.c_str() : "?", sec ? sec->name.c_str() : "?", bindName.empty() ? "" : " GOT=",
                       bindName.c_str(), sym && sym->addr == r.dst ? " sym=" : "",
                       sym && sym->addr == r.dst ? sym->name.c_str() : "");
            }
        }
        return 2;
    }
    if (cmd == "ptrsto" && argc >= 2)
    {
        uint64_t a = ParseNum(argv[1]);
        PrintPtrs(a, a + 1);
        return 2;
    }
    if (cmd == "ptrsat" && argc >= 3)
    {
        uint64_t a = ParseNum(argv[1]);
        uint64_t cnt = ParseNum(argv[2]);
        std::unordered_map<uint64_t, uint64_t> byLoc;
        for (auto& p : g_ptrs)
            if (p.loc >= a && p.loc < a + cnt * 8)
                byLoc[p.loc] = p.dst;
        std::unordered_map<uint64_t, uint32_t> bindByLoc;
        for (auto& b : g_binds)
            if (b.loc >= a && b.loc < a + cnt * 8)
                bindByLoc[b.loc] = b.ordinal;
        for (uint64_t i = 0; i < cnt; i++)
        {
            uint64_t loc = a + i * 8;
            auto it = byLoc.find(loc);
            auto bt = bindByLoc.find(loc);
            if (it != byLoc.end())
            {
                const Section* s = SectFor(it->second);
                std::string extra;
                if (s && s->name == "__text")
                    extra = FuncName(it->second);
                else if (InCstring(it->second))
                    extra = "\"" + CStr(it->second, 80) + "\"";
                else if (const Sym* sym = SymAt(it->second); sym && sym->addr == it->second)
                    extra = sym->name;
                printf("  [%3" PRIu64 "] +%#5" PRIx64 " %#" PRIx64 " -> %#" PRIx64 " [%s] %s\n", i, i * 8, loc, it->second,
                       s ? s->name.c_str() : "?", extra.c_str());
            }
            else if (bt != bindByLoc.end())
                printf("  [%3" PRIu64 "] +%#5" PRIx64 " %#" PRIx64 " bind %s\n", i, i * 8, loc,
                       bt->second < g_importNames.size() ? g_importNames[bt->second].c_str() : "?");
            else
            {
                const uint8_t* p = VmToFile(loc);
                uint64_t raw = 0;
                if (p)
                    memcpy(&raw, p, 8);
                printf("  [%3" PRIu64 "] +%#5" PRIx64 " %#" PRIx64 " raw %#018" PRIx64 "\n", i, i * 8, loc, raw);
            }
        }
        return 3;
    }
    if (cmd == "gotname" && argc >= 2)
    {
        std::regex re(argv[1]);
        int shown = 0;
        for (auto& b : g_binds)
        {
            if (b.ordinal >= g_importNames.size())
                continue;
            const auto& nm = g_importNames[b.ordinal];
            if (!std::regex_search(nm, re))
                continue;
            const Section* s = SectFor(b.loc);
            printf("bind %#" PRIx64 " [%s] = %s\n", b.loc, s ? s->name.c_str() : "?", nm.c_str());
            PrintCodeRefs(b.loc, b.loc + 1, "    ");
            if (++shown > 300)
                break;
        }
        return 2;
    }
    if (cmd == "sym" && argc >= 2)
    {
        std::regex re(argv[1]);
        for (auto& s : g_syms)
            if (std::regex_search(s.name, re))
                printf("%#" PRIx64 " %s\n", s.addr, s.name.c_str());
        return 2;
    }
    if (cmd == "symat" && argc >= 2)
    {
        uint64_t a = ParseNum(argv[1]);
        const Sym* s = SymAt(a);
        if (s)
            printf("%#" PRIx64 " %s (+%#" PRIx64 ")\n", s->addr, s->name.c_str(), a - s->addr);
        return 2;
    }
    if (cmd == "imm" && argc >= 2)
    {
        uint32_t v = uint32_t(ParseNum(argv[1]));
        auto it = std::lower_bound(g_imms.begin(), g_imms.end(), v, [](const Imm& i, uint32_t x) { return i.value < x; });
        int cnt = 0;
        for (; it != g_imms.end() && it->value == v && cnt < 300; ++it, ++cnt)
            printf("  %#" PRIx64 " in %s\n", it->src, FuncName(it->src).c_str());
        return 2;
    }
    if ((cmd == "reach" || cmd == "up") && argc >= 3)
    {
        // Call graph over function starts: BL and B edges (B to another function = tail call).
        static std::unordered_map<uint64_t, std::vector<uint64_t>> down, upg;
        if (down.empty())
        {
            for (auto& c : g_calls)
            {
                auto [fs, fe] = FuncOf(c.src);
                auto [ts, te] = FuncOf(c.dst);
                if (!fs || !ts || ts == fs)
                    continue;
                if (c.kind == 1 && c.dst != ts)
                    continue; // intra-function branch into the middle of another function: skip
                down[fs].push_back(ts);
                upg[ts].push_back(fs);
            }
        }
        if (cmd == "reach" && argc >= 4)
        {
            uint64_t from = FuncOf(ParseNum(argv[1])).first, to = FuncOf(ParseNum(argv[2])).first;
            int maxDepth = int(ParseNum(argv[3]));
            std::unordered_map<uint64_t, uint64_t> prev;
            std::vector<uint64_t> frontier{from};
            prev[from] = 0;
            bool found = false;
            for (int d = 0; d < maxDepth && !found && !frontier.empty(); d++)
            {
                std::vector<uint64_t> next;
                for (auto f : frontier)
                    for (auto t : down[f])
                        if (!prev.count(t))
                        {
                            prev[t] = f;
                            if (t == to)
                                found = true;
                            next.push_back(t);
                        }
                frontier.swap(next);
            }
            if (!found)
            {
                printf("no path within depth %d\n", maxDepth);
                return 4;
            }
            std::vector<uint64_t> path;
            for (uint64_t x = to; x; x = prev[x])
                path.push_back(x);
            for (auto it = path.rbegin(); it != path.rend(); ++it)
                printf("  -> %s\n", FuncName(*it).c_str());
            return 4;
        }
        if (cmd == "up")
        {
            uint64_t start = FuncOf(ParseNum(argv[1])).first;
            int maxDepth = int(ParseNum(argv[2]));
            std::unordered_map<uint64_t, int> seen;
            std::function<void(uint64_t, int)> rec = [&](uint64_t f, int d) {
                printf("%*s%s%s\n", d * 2, "", FuncName(f).c_str(), seen.count(f) ? " (seen)" : "");
                if (seen.count(f) || d >= maxDepth)
                    return;
                seen[f] = d;
                auto& v = upg[f];
                std::vector<uint64_t> u(v.begin(), v.end());
                std::sort(u.begin(), u.end());
                u.erase(std::unique(u.begin(), u.end()), u.end());
                int shown = 0;
                for (auto c : u)
                {
                    if (++shown > 12)
                    {
                        printf("%*s... %zu callers\n", (d + 1) * 2, "", u.size());
                        break;
                    }
                    rec(c, d + 1);
                }
            };
            rec(start, 0);
            return 3;
        }
    }
    fprintf(stderr, "unknown command or missing args: %s\n", cmd.c_str());
    return argc;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <binary> <command> [args] [; command args ...]\n", argv[0]);
        return 2;
    }
    Load(argv[1]);
    int i = 2;
    while (i < argc)
    {
        if (strcmp(argv[i], ";") == 0)
        {
            i++;
            continue;
        }
        int j = i;
        while (j < argc && strcmp(argv[j], ";") != 0)
            j++;
        printf("== %s\n", argv[i]);
        Run(j - i, argv + i);
        fflush(stdout);
        i = j;
    }
    return 0;
}
