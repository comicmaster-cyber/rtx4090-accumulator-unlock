# 03 — 진단 도구 사용법

## 1. Raw MMA 측정

```text
cd tools\benchmark
build.cmd
accum.exe
```

- CUDA Toolkit 필요, `-arch=sm_89`
- 워밍업 후 5회 측정
- 두 번 실행해 두 번째 값을 사용 (클럭 램프)

판정:

| 관찰 | 의미 |
|---|---|
| `f32acc` ≈ `f16acc` | 정상(복구된) 상태 |
| `f32acc` ≈ `f16acc`의 절반 | `docs/01`의 축소 상태 |
| 둘 다 낮음 | 클럭 램프/전력/열 문제 가능 |

전력이 200 W대여도 정상입니다. 텐서 전용 루프는 TGP를 다 쓰지 않습니다.

## 2. 콜드 상태 캡처 (읽기 전용)

```text
tools\probe\build_probe.cmd
```

1. FAT32 USB 루트에 `EFI\BOOT\BOOTX64.EFI` = `AD102-DUMP.EFI`
2. Secure Boot OFF, 모니터를 대상 GPU에 연결
3. USB 부팅, 종료 후 아무 키
4. USB 루트의 파일 회수:

```text
DUMP00001.LOG
ROM00001.BIN
PROM00001.BIN
```

프로브는 GPU 레지스터 쓰기, DMA, FLR, 드라이버 소유권 변경을 하지 않습니다.
BAR0가 미할당이면 아무것도 쓰지 않고 건너뜁니다. 유일한 예외는 PCI command의
memory-decode 비트가 꺼져 있고 BAR0가 유효할 때 그 비트만 켜는 것입니다.

## 3. 캡처 요약

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

출력:
- device/subsystem/chip/VRAM
- 관찰된 콜드 상태 (셀렉터/권한/WPR)
- ROM/PROM 크기와 SHA-256

`profiles/observed.json`과 비교해 카드가 같은 계열인지 확인합니다.
이 도구는 **목표값이나 절차를 출력하지 않습니다.**

## 4. 개인정보/배포 주의

- `DUMP*.LOG`, `ROM*.BIN`, `PROM*.BIN`은 **기계별 데이터**입니다.
- BAR 주소, PCI config, 카드의 ROM/PROM 내용이 포함됩니다.
- Git에 커밋하지 마세요 (`.gitignore`에 포함).
- 공유 전에는 식별자를 제거하거나 해시만 남기세요.

## 5. 문제 해결

| 증상 | 확인 |
|---|---|
| 프로브가 실행되지 않음 | Secure Boot OFF, FAT32, `EFI\BOOT\BOOTX64.EFI` 경로 |
| `REGS skipped: no valid BAR0` | BIOS에서 GPU BAR 할당/Above 4G 확인 |
| `PROM read-error` | BAR0 크기/윈도 확인 |
| 로그가 비어 있음 | USB 쓰기 가능 여부, 볼륨 루트 |
| 벤치가 첫 실행에서 낮음 | 10초 워밍업 후 재실행 |
