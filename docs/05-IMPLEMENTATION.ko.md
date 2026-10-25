# 05 — LLM/에이전트용 구현 가이드

> 이 문서는 **재현 가이드**입니다. 이 저장소에는 코드·페이로드·EFI·펌웨어가 없습니다.
> 공개된 업스트림 구현을 가져와 **이 카드 프로파일**로 맞추는 방법을 설명합니다.
> 법적 리스크가 높습니다. 먼저 `docs/04-SAFETY-LEGAL.ko.md`를 읽으세요.

## 0. 전제

- 대상 카드: `profiles/observed.json`의 식별자/콜드 상태
- 콜드 부팅, **Secure Boot OFF**, **Fast Startup OFF**
- 모니터를 대상 GPU에 연결
- Windows ESP에 `bootmgfw.efi`가 정확히 1개
- Python 3.11+, **Zig 0.14.1**, (프로브 사용 시) MSVC
- USB 1개 (전체 삭제)

## 1. 업스트림 (공개)

| 저장소 | 고정 커밋 | 역할 |
|---|---|---|
| `Zagitta/gpu-unlock` | `0ebdb1c6308b6a2072be5bf04513472942541956` | 원본 리서치 코어 (GPL-2.0) |
| `WildFlash1st/cmp90hx-unlock` | `76940ae583281daa2b883518a2e89b56c11d3a46` | V67 서명 참조 (rejoin15 0014) |
| `allurerr/rtx4090-48gb-unlock` | `fd8fcb14dc5a6ed5ce9eea40e4cbb453c800df80` | UEFI/AD102 포트, 자산 생성기 |

```text
git clone https://github.com/allurerr/rtx4090-48gb-unlock.git
cd rtx4090-48gb-unlock
git checkout fd8fcb14dc5a6ed5ce9eea40e4cbb453c800df80
```

## 2. 프로파일 적응

업스트림은 **48GB 카드** 프로파일입니다. 이 카드(24GB)에 맞게 다음을 바꿉니다.
값은 `profiles/observed.json`에서 가져옵니다.

| 파일 | 항목 |
|---|---|
| `src/unlock/port.h` | device, subsystem, chip, VRAM, SS0/SS1 cold |
| `src/unlock/prom_profile.h` | PROM view 크기/해시 |
| `src/unlock/app.c` | PCI ROM 크기/해시, VRAM 허용값, selector pair |
| `profiles/card.json` | 위 값과 일치하도록 갱신 |

**코어 로직(SEC2/FWSEC/Booter/V67/FLR)은 수정하지 않습니다.**
셀렉터 목표값과 서명 처리도 업스트림 값을 그대로 사용합니다.

## 3. 자산 생성

```text
python -m pip install --only-binary=:all: --target local/python-deps zstandard==0.25.0
python tools/setup_firmware.py --download
```

- NVIDIA kernel-module-source **610.43.03**
- CachyOS nvidia-utils **610.43.03**
- V67: `tools/firmware/ad102_payload.py`
- 결과 6개 자산이 `profiles/assets.json` 해시와 일치해야 합니다.

## 4. 빌드

```text
python tools/build.py --zig zig
```

- Zig **0.14.1** 필수
- 출력: `payload/PREFLIGHT.EFI`, `payload/UNLOCK.EFI`
- PE/AMD64/EFI application, import 없음, relocation 존재 확인

## 5. 부팅 미디어

- GPT + **정확히 512MiB (1,048,576 sectors)** FAT32
- 레이아웃:

```text
EFI\BOOT\BOOTX64.EFI                 PREFLIGHT (첫 부팅)
payload\PREFLIGHT.EFI
payload\UNLOCK.EFI
RTX4090-48GB-AUTO\ASSETS\            자산 5개
RTX4090-48GB-AUTO\STATE.BIN          96B, "4090AUT1", byte[8]=0
```

## 6. 실행 순서

1. **Preflight** (읽기 전용): 부팅 후 로그 확인
   - chip, 콜드 상태, ROM/PROM 해시, 자산, Windows preload
   - `PREFLIGHT_ONLY PASS`, `FINAL status=0 ... clean=1`
2. **Unlock**: `EFI\BOOT\BOOTX64.EFI`를 `UNLOCK.EFI`로 교체, `STATE.BIN[8]=0`
3. 부팅 → 3초 대기(또는 `U`) → 화면 검게 → FLR → Windows
4. **전원 차단 전에 측정** (`tools/benchmark`)

## 7. 검증 기준

```text
transaction result=0
FLR=1
SELECTOR ... matched=1
POST-GOP ... SS0/SS1 기대값
FINAL status=0 ... clean=1
f32acc ≈ f16acc
```

- `STOP: cleanup cannot be proven` → **물리 전원 차단**, Windows 강제 진행 금지
- 실패 후 재시도 루프 금지

## 8. LLM 체크리스트

- [ ] 업스트림을 **고정 커밋**으로 받았는가
- [ ] `profiles/observed.json` 값으로 프로파일을 갱신했는가
- [ ] 코어 로직을 수정하지 않았는가
- [ ] 자산 6개 해시가 일치하는가
- [ ] Zig 0.14.1로 빌드했는가
- [ ] preflight를 먼저 통과했는가
- [ ] 전원 차단 전에 측정했는가
- [ ] 펌웨어/ROM/PROM/로그를 커밋하지 않았는가
- [ ] `python tools/privacy_scan.py`가 통과하는가

## 9. 법적/안전

- 이 가이드는 **높은 법적 리스크**를 수반합니다 (기술적 보호조치 우회, 수출통제, EULA).
- 이 저장소는 코드/펌웨어를 배포하지 않지만, 재현 가이드는 여전히 규제 대상이 될 수 있습니다.
- 사용/공개 전에 자격 있는 법률 검토를 받으세요. 이 문서는 법률 자문이 아닙니다.
