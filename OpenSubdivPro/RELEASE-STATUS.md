# 실사용본 / 개발본 구분 (2026-09-22)

## 현재 실사용 대상으로 유지한 파일

`C:\Users\ksi\AppData\Local\Autodesk\3dsMax\2027 - 64bit\ENU\3dsMaxPlugins\OpenSubdivPro\bin\Hybrid\OpenSubdivPro.dlm`

- 하드 엣지 추가 전 백업과 SHA-256 일치 확인.
- SHA-256: `4811EAAFA681A4C230E575540EADAE4AE8EBB95DCFD618DFA9FC0A29FEA22564`
- 기존 Weighted Normals 및 기존 기능 유지. 이번 정리에서 DLL을 덮어쓰지 않음.
- 등록된 플러그인 경로는 위 Hybrid 폴더. 현재 실행 프로세스에 로드된 바이너리는 이번 정리에서 확인하지 않음.
- 해시 확인은 전체 기능 실행 검증을 뜻하지 않음.

## 실사용 배포에서 제외한 개발 기능

- 순정 Weighted Normals 전체 설정 연결 및 새 Display Normals 구현.
- Weighted Normals 상단 오른쪽 ON/OFF UI.
- 새 Display Hard Edges 및 색상 선택.

위 기능은 빌드 기록만으로 완료로 간주하지 않는다. UI 응답 정지 및 로딩 시간 초과 기록이 있으며 실제 표시·저장·복원 검증이 미완료다.
HardEdges, NativeHardEdges, NativeWeightedNormals, WeightedNormalsToggle, SafeNormals, SafeTest 및 validation 폴더의 DLL은 실사용 설치 경로에 복사하지 않는다.

## 개발 재개 시 주의

현재 OpenSubdivPro.cpp는 개발 중 소스이며 위 실사용 DLL과 일치하는 소스로 확정되지 않았다.
이 소스를 Hybrid 출력 경로로 빌드하여 실사용 DLL을 덮어쓰지 않는다. 별도 출력 경로에서 검증한다.
개발 소스와 테스트 산출물은 삭제하지 않고 보존했다.