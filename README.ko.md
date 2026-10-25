# RTX 4090 24GB FP32-accumulate notes & diagnostics

[한국어](README.ko.md) | [English](README.md)

특정 **RTX 4090 24GB**에서 관찰된 **FP32 누산 텐서 처리량 절반** 현상을 측정하고,
읽기 전용으로 콜드 상태를 캡처하는 진단 도구 모음입니다.

> **이 저장소는 완성된 언락 도구가 아닙니다.** 펌웨어·코드·EFI·페이로드는 포함하지
> 않습니다. 진단 도구는 읽기 전용이고 `docs/02`는 고수준 설명입니다.
> **LLM/에이전트용 재현 가이드는 `docs/05`** 에 있으며, 공개 업스트림을 이 카드
> 프로파일에 맞추는 방법을 설명합니다.

## 관찰 요약

레퍼런스 카드(RTX 4090 24GB, AD102-300) 실측:

| 경로 | 실측 | 피크 대비 |
|---|---|---|
| FP16 → FP16 acc | 353.8 TFLOPS | 99.4% |
| FP16 → FP32 acc | 182.9 TFLOPS | 49.6% |
| BF16 → FP32 acc | 182.7 TFLOPS | 49.6% |
| TF32 → FP32 acc | 91.4 TFLOPS | 49.6% |

- 부하 중 P2, 2805 MHz, 162 W → 전력/클럭/열 문제 아님
- 텐서 코어는 정상, **FP32 누산 경로만 약 50%**
- 자세한 내용: `docs/01`

## 구성

```text
docs/01-OBSERVATION.ko/en.md    현상·측정 방법·결과
docs/02-NOTES.ko/en.md          고수준 아키텍처 (구현/절차 없음)
docs/03-DIAGNOSTICS.ko/en.md    프로브/벤치 사용법
docs/04-SAFETY-LEGAL.ko/en.md   안전·개인정보·법적 고지
docs/05-IMPLEMENTATION.ko/en.md ★ LLM/에이전트용 재현 가이드
profiles/observed.json          관찰된 카드 식별/콜드 상태
tools/benchmark/accum.cu        raw MMA (f32acc vs f16acc)
tools/probe/                    AD102-DUMP (읽기 전용 캡처)
tools/derive_profile.py         캡처 요약 (식별/해시)
tools/privacy_scan.py           개인정보/식별자 스캔
deps/edk2/MdePkg/Include        EDK2 헤더 (BSD)
```

## 빠른 시작

### 1) 처리량 측정

```text
cd tools\benchmark
build.cmd
accum.exe
```

`f32acc`가 `f16acc`의 절반이면 `docs/01`의 축소 상태입니다.

### 2) 콜드 상태 캡처 (읽기 전용)

```text
tools\probe\build_probe.cmd
```

FAT32 USB의 `EFI\BOOT\BOOTX64.EFI`에 `AD102-DUMP.EFI`를 넣고 부팅합니다.
Secure Boot OFF, 모니터를 대상 GPU에 연결. 결과:

```text
DUMP00001.LOG / ROM00001.BIN / PROM00001.BIN
```

### 3) 요약

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

## 요구사항

- Windows 11 x64 (프로브는 UEFI, Secure Boot OFF)
- Python 3.11+ (요약/스캔)
- CUDA Toolkit (벤치)
- Visual Studio C++ 빌드 도구 (프로브)
- EDK2 헤더는 저장소에 포함

## 안전

- 프로브는 GPU 레지스터 쓰기/DMA/FLR을 하지 않습니다.
- 벤치는 읽기 전용 측정이며 장시간 스트레스 테스트가 아닙니다.
- 로그/ROM/PROM은 기계별 데이터이므로 Git에 커밋하지 마세요.

## 법적 고지 / Disclaimer

이 저장소는 **NVIDIA와 무관**하며 승인·후원을 받지 않습니다. 코드는 **있는 그대로**
제공되고, 사용은 **전적으로 사용자의 책임**입니다. 저자와 기여자는 물리적 손상,
데이터 손실, 보증 상실, 시스템 불안정, 라이선스·계약·수출통제·지역 법규 위반,
직접·간접·특별·징벌적 손해에 대해 책임을 지지 않습니다.

기술적 보호조치 우회, 수출통제, EULA 이슈가 적용될 수 있습니다. 이 문서는 법률
자문이 아니며, 공개/사용 전에 자격 있는 법률 검토를 받으세요.

## 구현 가이드

`docs/05-IMPLEMENTATION.ko.md`는 공개 업스트림(Zagitta/gpu-unlock,
WildFlash1st/cmp90hx-unlock, allurerr/rtx4090-48gb-unlock)을 고정 커밋으로 받아
이 카드 프로파일(`profiles/observed.json`)에 맞추는 절차를 설명합니다.
이 저장소에는 코드/페이로드/EFI가 없으며, 가이드는 업스트림을 가리킵니다.

## 라이선스

- `tools/`의 원본 코드: **MIT** (`LICENSE`)
- `deps/edk2/MdePkg/Include`: **BSD-2-Clause-Patent** (`LICENSES/`)

영문: [`README.md`](README.md), `docs/*.en.md`, `AGENTS.md`.
