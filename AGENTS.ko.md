# AGENTS.ko.md — LLM/에이전트용 지침

이 저장소는 RTX 4090 24GB의 **FP32 누산 절반 현상을 측정·진단하는 읽기 전용**
패키지입니다. 이 범위를 유지하세요.

## 0. 범위

허용:
- raw `mma.sync` 처리량 측정 (`tools/benchmark`)
- 콜드 상태 읽기 전용 캡처 (`tools/probe`)
- 캡처 요약 (`tools/derive_profile.py`)
- 문서, 해시, 관찰값
- 공개 업스트림을 가리키는 재현 가이드 (`docs/05`)

금지:
- 언락 코드, 펌웨어 실행, 서명 처리, 레지스터 쓰기 추가
- 목표 레지스터 값, 페이로드, EFI, 코드를 직접 포함하는 것
- 펌웨어/ROM/PROM/로그/스크린샷 추가
- 개인정보/기계 식별자 추가

`docs/02`는 개념 설명이고, 재현 가이드는 `docs/05`에서 업스트림을 가리킵니다.
코드나 목표값을 이 repo에 넣지 마세요.

## 1. 절대 규칙

1. **읽기 전용.** GPU 레지스터 쓰기, 벤더 펌웨어 실행, DMA, FLR 코드를 추가하지 마세요.
2. **벤더 펌웨어 금지.** NVIDIA 펌웨어, vBIOS/ROM/PROM, EFI, 생성 자산을 커밋하지 마세요.
3. **절차 금지.** 정확한 주소/값/서명 단계/자산 레시피를 문서화하지 마세요.
4. **개인정보 금지.** 사용자명, 홈 경로, GPU UUID, 시리얼, SID, 부팅 항목, 호스트명 금지.
5. **커밋 전 스캔.** `python tools/privacy_scan.py`가 통과해야 합니다.
6. 측정하지 않은 애플리케이션 가속을 주장하지 마세요.

## 2. 워크플로

### 측정

```text
cd tools\benchmark
build.cmd
accum.exe
```

`f32acc`, `f16acc`, `bf16`, SM 클럭을 보고합니다. 두 번 실행해 두 번째 값을 사용하세요.
`f32acc`가 `f16acc`의 절반이면 축소 상태입니다.

### 캡처 (선택, 읽기 전용)

```text
tools\probe\build_probe.cmd
```

`AD102-DUMP.EFI`를 FAT32 USB의 `EFI\BOOT\BOOTX64.EFI`에 복사합니다. Secure Boot OFF,
모니터를 대상 GPU에 연결. **부팅은 사용자가 직접** 수행합니다.

### 요약

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

`profiles/observed.json`과 비교하세요. 관찰값을 목표값으로 바꾸지 마세요.

## 3. 구현을 요청받으면

`docs/05`를 따르세요.
- 고정 커밋의 업스트림을 받습니다 (코드를 이 repo에 복사하지 않음)
- `profiles/observed.json` 값으로 프로파일을 갱신합니다
- Zig 0.14.1로 업스트림을 빌드합니다
- preflight(읽기 전용)를 먼저 통과한 뒤 unlock, 전원 차단 전에 측정합니다

코드/펌웨어/페이로드/목표값/EFI를 이 repo에 추가하지 마세요. 가이드는 의도적으로
업스트림을 가리킵니다. 기술적 보호조치/수출통제/EULA 리스크와 법률 검토 필요성을
사용자에게 알리세요.

## 4. 보고 형식

```text
GPU: <이름 / subsystem / VBIOS>
Clock: <MHz>  Power: <W>  Temp: <C>
f32acc: <TFLOPS>
f16acc: <TFLOPS>
bf16:   <TFLOPS>
Ratio f32/f16: <x>
Cold state: <캡처 파일 / 미캡처>
Notes: <워밍업, 드라이버, CUDA 버전>
```

## 5. 파일 맵

```text
docs/01-OBSERVATION.*     현상·측정·결과
docs/02-NOTES.*           고수준 아키텍처 (구현 없음)
docs/03-DIAGNOSTICS.*     도구 사용법
docs/04-SAFETY-LEGAL.*    안전·개인정보·법적 고지
profiles/observed.json    관찰된 식별/콜드 상태
tools/benchmark/          raw MMA 측정
tools/probe/              읽기 전용 UEFI 캡처
tools/derive_profile.py   캡처 요약
tools/privacy_scan.py     개인정보 스캔
deps/edk2/MdePkg/Include  고정 EDK2 헤더 (BSD)
```

## 6. 라이선스

원본 코드는 MIT. EDK2 헤더는 BSD-2-Clause-Patent. NVIDIA 펌웨어 미포함.
NVIDIA와 무관합니다.
