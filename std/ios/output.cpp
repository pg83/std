#include "output.h"
#include "unbound.h"
#include "manip.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/range.h>

using namespace Std;
using namespace Std::Manip;

Output::~Output() {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

void Output::writeVImpl(const StringView* parts, size_t count) {
    for (const auto& it : range(parts, parts + count)) {
        write(it.data(), it.length());
    }
}

ZeroCopyOutput::~ZeroCopyOutput() {
}

void ZeroCopyOutput::writeImpl(const void* data, size_t len) {
    bump(imbue(len) << StringView((const u8*)data, len));
}

// modifiers
template <>
void Std::output<ZeroCopyOutput, Flush>(ZeroCopyOutput& out, Flush) {
    out.flush();
}

template <>
void Std::output<ZeroCopyOutput, Finish>(ZeroCopyOutput& out, Finish) {
    out.finish();
}

template <>
void Std::output<ZeroCopyOutput, EndLine>(ZeroCopyOutput& out, EndLine) {
    out << u8'\n';
}

// strings
template <>
void Std::output<ZeroCopyOutput, const u8*>(ZeroCopyOutput& out, const u8* str) {
    out.write(str, strLen(str));
}

template <>
void Std::output<ZeroCopyOutput, u8>(ZeroCopyOutput& out, u8 ch) {
    out.write(&ch, 1);
}

// std types - unsigned
template <>
void Std::output<ZeroCopyOutput, u16>(ZeroCopyOutput& out, u16 v) {
    out << (u64)v;
}

template <>
void Std::output<ZeroCopyOutput, u32>(ZeroCopyOutput& out, u32 v) {
    out << (u64)v;
}

template <>
void Std::output<ZeroCopyOutput, u64>(ZeroCopyOutput& out, u64 v) {
    out.bump(out.imbue(24) << v);
}

// std types - signed
template <>
void Std::output<ZeroCopyOutput, i16>(ZeroCopyOutput& out, i16 v) {
    out << (i64)v;
}

template <>
void Std::output<ZeroCopyOutput, i32>(ZeroCopyOutput& out, i32 v) {
    out << (i64)v;
}

template <>
void Std::output<ZeroCopyOutput, i64>(ZeroCopyOutput& out, i64 v) {
    out.bump(out.imbue(24) << v);
}
