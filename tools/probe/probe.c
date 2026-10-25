/*
 * AD102-DUMP v1 -- read-only cold-state capture for diagnostics.
 * It performs no GPU MMIO writes, no DMA, no FLR and no ownership changes.
 * The only possible PCI config change is enabling the memory-decode bit when
 * it is off and BAR0 is already assigned (the same thing an OS does at boot).
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <IndustryStandard/Acpi10.h>

#define PROBE_VER "AD102-DUMP v1 read-only"

static EFI_SYSTEM_TABLE  *ST;
static EFI_BOOT_SERVICES *BS;
static EFI_FILE_PROTOCOL *LogFile;
static EFI_FILE_PROTOCOL *Root;
static CHAR16             LogName[16];
static CHAR16             RomName[16];
static CHAR16             PromName[16];

/* Local GUID copies: no GUID library is linked. */
static EFI_GUID mPciIoGuid   = {0x4cf5b200,0x68b8,0x4ca5,{0x9e,0xec,0xb2,0x3e,0x3f,0x50,0x02,0x9a}};
static EFI_GUID mLoadedGuid  = {0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static EFI_GUID mFsGuid      = {0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};

/* ------------------------------------------------------------------ text */
static CHAR8 Line[600];
static UINTN LineLen;

static VOID LReset(VOID) { LineLen = 0; }
static VOID LCh(CHAR8 c) { if (LineLen < sizeof(Line) - 2) Line[LineLen++] = c; }
static VOID LStr(const CHAR8 *s) { while (*s) LCh(*s++); }
static VOID LHexN(UINT64 v, UINTN digits)
{
    static const CHAR8 h[] = "0123456789abcdef";
    INTN i;
    for (i = (INTN)digits - 1; i >= 0; i--) LCh(h[(v >> (i * 4)) & 0xf]);
}
static VOID LHex64(UINT64 v) { LStr("0x"); LHexN(v, 16); }
static VOID LHex32(UINT32 v) { LStr("0x"); LHexN(v, 8); }
static VOID LDec(UINT64 v)
{
    CHAR8 t[24]; UINTN n = 0;
    if (!v) { LCh('0'); return; }
    while (v) { t[n++] = (CHAR8)('0' + (v % 10)); v /= 10; }
    while (n) LCh(t[--n]);
}
static VOID LFlush(VOID)
{
    if (LogFile && LineLen) {
        UINTN n = LineLen;
        LogFile->Write(LogFile, &n, Line);
        LogFile->Flush(LogFile);
    }
    LineLen = 0;
}
static VOID LOut(VOID) { LCh('\r'); LCh('\n'); LFlush(); }

static VOID ConStr(const CHAR8 *s)
{
    CHAR16 b[256]; UINTN n = 0;
    if (!ST || !ST->ConOut) return;
    while (*s && n < 254) b[n++] = (CHAR16)(UINT8)*s++;
    b[n] = 0;
    ST->ConOut->OutputString(ST->ConOut, b);
}

/* --------------------------------------------------------------- filenames */
static VOID MakeName(CHAR16 *dst, const CHAR8 *base, UINT32 seq, const CHAR8 *ext)
{
    UINTN i = 0;
    CHAR8 num[6]; INTN k;
    while (*base) dst[i++] = (CHAR16)(UINT8)*base++;
    num[5] = 0;
    for (k = 4; k >= 0; k--) { num[k] = (CHAR8)('0' + (seq % 10)); seq /= 10; }
    for (k = 0; k < 5; k++) dst[i++] = (CHAR16)(UINT8)num[k];
    while (*ext) dst[i++] = (CHAR16)(UINT8)*ext++;
    dst[i] = 0;
}

static BOOLEAN Exists(EFI_FILE_PROTOCOL *root, CHAR16 *name)
{
    EFI_FILE_PROTOCOL *f = NULL;
    EFI_STATUS s = root->Open(root, &f, name, EFI_FILE_MODE_READ, 0);
    if (f) f->Close(f);
    return !EFI_ERROR(s);
}

static EFI_STATUS Create(EFI_FILE_PROTOCOL *root, CHAR16 *name, EFI_FILE_PROTOCOL **out)
{
    return root->Open(root, out, name,
                      EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
}

/* ------------------------------------------------------------------- PCI */
static EFI_STATUS RegRead(EFI_PCI_IO_PROTOCOL *io, UINT32 off, UINT32 *val)
{
    return io->Mem.Read(io, EfiPciIoWidthUint32, 0, off, 1, val);
}

static const UINT32 RegList[] = {
    0x00000000u, /* PMC_BOOT_0 / chip id                         */
    0x00000100u,
    0x0001180f8u,
    0x000118128u,
    0x000118234u,
    0x0001183a4u, /* usable FB in MiB                             */
    0x000110000u,
    0x0001100f4u,
    0x000110100u, /* FWSEC CPUCTL                                 */
    0x000110104u,
    0x000110600u,
    0x000625f04u,
    0x0001fa7c0u,
    0x0001fa7c4u, /* WPR                                          */
    0x0001fa7c8u, /* PLM                                          */
    0x0001fa7ccu, /* WPR_CFG                                      */
    0x0001fa824u, /* WPR2 low                                     */
    0x0001fa828u, /* WPR2 high                                    */
    0x0001fa82cu, /* MMU lock low                                 */
    0x0001fa830u, /* MMU lock high                                */
    0x0009a0148u, /* FBPA                                         */
    0x000823804u, /* SS0: eight 4-bit selectors                   */
    0x000823808u,
    0x00082380cu,
    0x00082381cu, /* SS1                                          */
    0x000823820u,
    0x000823824u,
    0x000823b00u, /* SOURCE_ENABLE candidate (diagnostic only) */
    0x000824140u, /* SEC2 ucode fuses 0..3                        */
    0x000824144u,
    0x000824148u,
    0x00082414cu,
    0x0008241e0u,
    0x000840000u,
    0x000840100u,
    0x0008403c0u,
    0x0008403c4u,
    0x000841000u,
    0x000841668u,
    0x0001438u,   /* FWSEC scratch word                           */
};

static VOID DumpFuses(EFI_PCI_IO_PROTOCOL *io)
{
    UINT32 i, v;
    LReset(); LStr("FU "); LOut();
    for (i = 0; i < 16; i++) {
        UINT32 off = 0x0008241c0u + i * 4u;
        if (EFI_ERROR(RegRead(io, off, &v))) { LReset(); LStr("FU read-error off="); LHex32(off); LOut(); continue; }
        LReset(); LStr("FU "); LHex32(off); LCh(' '); LHex32(v); LOut();
    }
}

static VOID DumpConfig(EFI_PCI_IO_PROTOCOL *io)
{
    UINT32 buf[64];
    UINTN i;
    if (!EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint32, 0, 64, buf))) {
        for (i = 0; i < 64; i++) {
            LReset(); LStr("CFG "); LHex32((UINT32)(i * 4)); LCh(' '); LHex32(buf[i]); LOut();
        }
    } else {
        LReset(); LStr("CFG read-error 0x00"); LOut();
    }
    if (!EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint32, 0x100, 64, buf))) {
        for (i = 0; i < 64; i++) {
            LReset(); LStr("CFGX "); LHex32((UINT32)(0x100 + i * 4)); LCh(' '); LHex32(buf[i]); LOut();
        }
    } else {
        LReset(); LStr("CFGX read-error 0x100"); LOut();
    }
}

static VOID HexDump(const CHAR8 *tag, const UINT8 *p, UINTN n)
{
    UINTN i, j;
    for (i = 0; i < n; i += 16) {
        LReset(); LStr(tag); LCh(' '); LHexN(i, 4); LStr(": ");
        for (j = 0; j < 16 && i + j < n; j++) { LHexN(p[i + j], 2); LCh(' '); }
        LOut();
    }
}

static VOID WriteBin(CHAR16 *name, const UINT8 *data, UINTN size)
{
    EFI_FILE_PROTOCOL *f = NULL;
    if (EFI_ERROR(Create(Root, name, &f)) || !f) {
        LReset(); LStr("BIN create failed"); LOut();
        return;
    }
    UINTN n = size;
    EFI_STATUS s = f->Write(f, &n, (VOID *)data);
    f->Flush(f);
    f->Close(f);
    LReset(); LStr("BIN "); LStr("written size="); LDec(size); LStr(" status="); LHex64(s); LOut();
}

static VOID SizeLine(const CHAR8 *tag, UINTN size)
{
    LReset(); LStr(tag); LStr(" size="); LDec(size); LOut();
}

/* --------------------------------------------------------------- device */
static VOID ProbeDevice(EFI_PCI_IO_PROTOCOL *io, UINTN seg, UINTN bus, UINTN dev, UINTN fn)
{
    UINT32 id = 0, v;
    UINTN i;
    UINT64 attr = 0;
    EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *bar = NULL;
    UINT64 barMin = 0, barLen = 0;
    BOOLEAN barOk = FALSE;
    UINT8 *prom = NULL;
    UINTN promLen = 0, romLen = 0;
    UINT8 *rom = NULL;

    LReset();
    LStr("DEV seg="); LDec(seg); LStr(" bus="); LDec(bus); LStr(" dev="); LDec(dev); LStr(" fn="); LDec(fn);
    if (EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint32, 0, 1, &id))) { LStr(" id-read-error"); LOut(); return; }
    LStr(" id="); LHex32(id); LOut();

    /* BAR0 must be assigned before any MMIO access. Enabling memory decode on
       an unassigned BAR0 would alias low memory, so that case is skipped. */
    if (!EFI_ERROR(io->GetBarAttributes(io, 0, NULL, (VOID **)&bar)) && bar) {
        if (bar->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
            barMin = bar->AddrRangeMin;
            barLen = bar->AddrLen;
            barOk = (barMin != 0 && barLen >= 0x100000);
            LReset(); LStr("BAR0 min="); LHex64(barMin);
            LStr(" max="); LHex64(bar->AddrRangeMax);
            LStr(" len="); LHex64(barLen);
            LStr(" gran="); LHex64(bar->AddrSpaceGranularity);
            LStr(barOk ? " VALID" : " NOT-VALID"); LOut();
        } else {
            LReset(); LStr("BAR0 unexpected-desc"); LOut();
        }
        BS->FreePool(bar);
        bar = NULL;
    } else {
        LReset(); LStr("BAR0 read-error"); LOut();
    }

    if (barOk) {
        io->Attributes(io, EfiPciIoAttributeOperationGet, 0, &attr);
        LReset(); LStr("ATTR before "); LHex64(attr); LOut();
        {
            UINT16 cmd = 0;
            if (!EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint16, 0x04, 1, &cmd))) {
                LReset(); LStr("PCICMD before "); LHex32(cmd); LOut();
                if (!(cmd & 0x0002)) {
                    EFI_STATUS es = io->Attributes(io, EfiPciIoAttributeOperationEnable,
                                                   EFI_PCI_IO_ATTRIBUTE_MEMORY, NULL);
                    LReset(); LStr("MEMDECODE enable status="); LHex64(es); LOut();
                    if (!EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint16, 0x04, 1, &cmd))) {
                        LReset(); LStr("PCICMD after "); LHex32(cmd); LOut();
                    }
                }
            }
        }
        io->Attributes(io, EfiPciIoAttributeOperationGet, 0, &attr);
        LReset(); LStr("ATTR after "); LHex64(attr); LOut();

        DumpConfig(io);

        for (i = 0; i < ARRAY_SIZE(RegList); i++) {
            if (EFI_ERROR(RegRead(io, RegList[i], &v))) {
                LReset(); LStr("REG "); LHex32(RegList[i]); LStr(" read-error"); LOut();
            } else {
                LReset(); LStr("REG "); LHex32(RegList[i]); LCh(' '); LHex32(v); LOut();
            }
        }
        DumpFuses(io);
    } else {
        LReset(); LStr("REGS skipped: no valid BAR0"); LOut();
    }

    rom = io->RomImage;
    romLen = (UINTN)io->RomSize;
    LReset(); LStr("ROM ptr="); LHex64((UINT64)(UINTN)rom); LStr(" len="); LDec(romLen); LOut();
    if (rom && romLen) {
        SizeLine("ROM", romLen);
        HexDump("ROMH", rom, 0x100);
        WriteBin(RomName, rom, romLen);
    }

    if (!barOk) {
        LReset(); LStr("PROM skipped: no valid BAR0"); LOut();
        return;
    }

    promLen = 0x9a200;
    LReset(); LStr("PROM begin offset=0x00300000 len="); LDec(promLen); LOut();
    if (!EFI_ERROR(BS->AllocatePool(EfiLoaderData, promLen, (VOID **)&prom)) && prom) {
        EFI_STATUS s = io->Mem.Read(io, EfiPciIoWidthUint32, 0, 0x300000u, promLen / 4, prom);
        LReset(); LStr("PROM read status="); LHex64(s); LOut();
        if (!EFI_ERROR(s)) {
            SizeLine("PROM", promLen);
            HexDump("PROMH", prom, 0x100);
            WriteBin(PromName, prom, promLen);
        }
        BS->FreePool(prom);
    } else {
        LReset(); LStr("PROM alloc failed"); LOut();
    }
}

/* ------------------------------------------------------------------ main */
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS s;
    EFI_LOADED_IMAGE_PROTOCOL *li = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    EFI_FILE_PROTOCOL *root = NULL;
    EFI_HANDLE *handles = NULL;
    UINTN count = 0, i;
    UINT32 seq;
    BOOLEAN found = FALSE;

    ST = SystemTable;
    BS = SystemTable->BootServices;

    s = BS->HandleProtocol(ImageHandle, &mLoadedGuid, (VOID **)&li);
    if (EFI_ERROR(s) || !li) return s;
    s = BS->HandleProtocol(li->DeviceHandle, &mFsGuid, (VOID **)&fs);
    if (EFI_ERROR(s) || !fs) return s;
    s = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(s) || !root) return s;
    Root = root;

    for (seq = 1; seq <= 99999; seq++) {
        MakeName(LogName, "DUMP", seq, ".LOG");
        if (!Exists(root, LogName)) break;
    }
    MakeName(RomName, "ROM", seq, ".BIN");
    MakeName(PromName, "PROM", seq, ".BIN");

    s = Create(root, LogName, &LogFile);
    if (EFI_ERROR(s) || !LogFile) return s;

    LReset(); LStr(PROBE_VER); LOut();
    ConStr(PROBE_VER);
    ConStr("\r\nscanning NVIDIA PCI devices...\r\n");

    s = BS->LocateHandleBuffer(ByProtocol, &mPciIoGuid, NULL, &count, &handles);
    if (EFI_ERROR(s)) {
        LReset(); LStr("LocateHandleBuffer failed "); LHex64(s); LOut();
    } else {
        LReset(); LStr("HANDLES "); LDec(count); LOut();
        for (i = 0; i < count; i++) {
            EFI_PCI_IO_PROTOCOL *io = NULL;
            UINTN seg = 0, bus = 0, dev = 0, fn = 0;
            UINT32 id = 0;
            if (EFI_ERROR(BS->HandleProtocol(handles[i], &mPciIoGuid, (VOID **)&io)) || !io) continue;
            if (EFI_ERROR(io->GetLocation(io, &seg, &bus, &dev, &fn))) continue;
            if (EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint32, 0, 1, &id))) continue;
            if ((id & 0xffff) != 0x10de) continue;
            LReset(); LStr("SCAN bus="); LDec(bus); LStr(" dev="); LDec(dev); LStr(" fn="); LDec(fn);
            LStr(" id="); LHex32(id); LStr(" romlen="); LDec((UINTN)io->RomSize); LOut();
            if (!found && bus == 1 && dev == 0 && fn == 0) {
                ProbeDevice(io, seg, bus, dev, fn);
                found = TRUE;
            }
        }
        if (!found) {
            /* Fallback: first NVIDIA function that exposes a ROM image. */
            for (i = 0; i < count; i++) {
                EFI_PCI_IO_PROTOCOL *io = NULL;
                UINTN seg = 0, bus = 0, dev = 0, fn = 0;
                UINT32 id = 0;
                if (EFI_ERROR(BS->HandleProtocol(handles[i], &mPciIoGuid, (VOID **)&io)) || !io) continue;
                if (EFI_ERROR(io->GetLocation(io, &seg, &bus, &dev, &fn))) continue;
                if (EFI_ERROR(io->Pci.Read(io, EfiPciIoWidthUint32, 0, 1, &id))) continue;
                if ((id & 0xffff) != 0x10de || !io->RomImage || !io->RomSize) continue;
                LReset(); LStr("FALLBACK"); LOut();
                ProbeDevice(io, seg, bus, dev, fn);
                found = TRUE;
                break;
            }
        }
        if (handles) BS->FreePool(handles);
    }

    LReset(); LStr("RESULT target_found="); LDec(found ? 1 : 0); LOut();
    LReset(); LStr("END"); LOut();
    LogFile->Flush(LogFile);
    LogFile->Close(LogFile);
    LogFile = NULL;

    ConStr("done. log written to \\DUMP#####.LOG on this USB volume.\r\n");
    ConStr("press any key to exit.\r\n");
    if (ST->ConIn) {
        EFI_INPUT_KEY key;
        UINTN idx;
        ST->ConIn->Reset(ST->ConIn, FALSE);
        BS->WaitForEvent(1, &ST->ConIn->WaitForKey, &idx);
        ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
    }
    return EFI_SUCCESS;
}
