# Animate C-Level Extensibility DLL

이 프로젝트는 Adobe Animate에서 로드할 수 있는 C-Level Extensibility DLL 예제입니다. DLL이 로드되면 Adobe Animate의 C-Level Extensibility API를 통해 JSFL에서 호출 가능한 네이티브 함수를 등록하고, 등록된 함수로 로컬 TCP 서버에 접속하거나 메시지를 주고받습니다.

현재 구현은 Windows / Visual Studio 기준이며, 소켓 통신은 Winsock을 사용합니다.

## 주요 기능

- **Animate C-Level Extensibility 진입점**
  - 실제 export 함수는 `jsapi.h`의 `MM_InitWrapper(MM_Environment* env, unsigned int envSize)`입니다.
  - `MM_InitWrapper`가 Animate에서 전달한 환경 포인터를 `mmEnv`에 복사한 뒤, 사용자 정의 `MM_Init()`를 호출합니다.
  - `MM_Init()`에서는 JSFL에서 호출할 수 있는 `JSNative` 함수들을 등록합니다.

- **JSFL 호출 함수 등록**
  - `startSocketClient()`: `127.0.0.1:50313`으로 TCP 접속을 시도합니다.
  - `sendMessage(message)`: JSFL 문자열 인자를 UTF-8 문자열로 변환한 뒤 소켓으로 전송합니다.
  - `getMessage(name)`: `PacketProcessor`에 저장된 메시지 중 `name` 키에 해당하는 값 목록을 JS 배열로 반환합니다.

- **TCP 클라이언트**
  - `SocketClient`는 싱글턴으로 구현되어 있습니다.
  - `Connect(port)` 호출 시 `127.0.0.1`의 지정 포트로 접속합니다.
  - 접속 후 별도 수신 스레드에서 `recv()`를 반복 호출하고, 받은 데이터를 `PacketProcessor`로 전달합니다.
  - 수신 데이터 처리 후 `"Done"` 메시지를 서버로 다시 전송합니다.

- **간단한 메시지 파싱**
  - `PacketProcessor`는 수신 문자열을 `name:value1,value2` 형식으로 가정합니다.
  - `:` 앞부분을 키로 사용하고, `:` 뒤쪽 값을 `,` 기준으로 나누어 저장합니다.
  - 같은 키로 데이터가 다시 들어오면 기존 목록 뒤에 값을 추가합니다.

## 구성 파일

- `Animate_C_Extension/main.cpp`
  - JSFL에서 호출할 C++ 함수(`JS_startSocketClient`, `JS_sendMessage`, `JS_getMessage`)를 정의합니다.
  - `MM_Init()`에서 `startSocketClient`, `sendMessage`, `getMessage`를 등록합니다.
  - `EXE_TEST|x64` 구성에서 사용할 수 있는 콘솔 테스트용 `main()`도 포함되어 있습니다.

- `Animate_C_Extension/jsapi.h`
  - Adobe Animate C-Level Extensibility API에 필요한 타입, 매크로, `MM_Environment`, `MM_InitWrapper`를 정의합니다.

- `Animate_C_Extension/SocketClient.h`, `Animate_C_Extension/SocketClient.cpp`
  - Winsock 초기화, TCP 연결/해제, 송신, 수신 스레드를 담당합니다.

- `Animate_C_Extension/PacketProcessor.h`, `Animate_C_Extension/PacketProcessor.cpp`
  - 수신된 문자열을 `:` 및 `,` 기준으로 파싱해 `unordered_map<string, vector<string>>`에 저장합니다.

- `Animate_C_Extension/myUtil.h`, `Animate_C_Extension/myUtil.cpp`
  - 문자열 split, `std::wstring` 포인터 변환, `std::wstring`에서 UTF-8 `std::string`으로 변환하는 유틸리티를 제공합니다.

## 빌드 구성

프로젝트 파일에는 다음 구성이 포함되어 있습니다.

- `Debug|x64`, `Release|x64`
  - `DynamicLibrary`로 빌드됩니다.
  - Animate에서 로드할 DLL을 만들 때 사용하는 구성입니다.

- `EXE_TEST|x64`
  - `Application`으로 빌드됩니다.
  - `main.cpp`의 콘솔 테스트용 `main()`을 실행할 수 있는 구성입니다.

- `Win32` 구성
  - 프로젝트 파일에는 남아 있지만, README 기준의 주 대상은 64비트 Animate에 맞춘 `x64` DLL입니다.

## 배포 위치

빌드된 DLL은 Animate의 External Libraries 폴더에 배치해야 합니다. 예:

```text
C:\Users\USER\AppData\Local\Adobe\Animate 2024\en_US\Configuration\External Libraries\
```

Animate가 DLL을 로드하면 export된 `MM_InitWrapper`를 통해 함수 등록이 수행됩니다.

## JSFL 사용 예

```js
Animate_C_Extension.startSocketClient();
Animate_C_Extension.sendMessage("hello");
var messages = Animate_C_Extension.getMessage("Test");
```

위 예시의 `Animate_C_Extension` 객체명은 DLL이 Animate에 로드될 때 노출되는 라이브러리 이름에 의존합니다. 실제 호출 이름은 배치된 DLL 이름과 Animate의 로딩 방식에 맞춰 확인해야 합니다.

## 통신 데이터 형식

현재 `PacketProcessor`는 다음과 같은 단순 문자열 형식을 기대합니다.

```text
Test:ProjectA,ProjectB,ProjectC
```

이 데이터가 수신되면 `getMessage("Test")` 호출 시 `["ProjectA", "ProjectB", "ProjectC"]` 형태의 JS 배열을 반환하도록 구성되어 있습니다.

## 현재 구현의 제약

- 저장소에는 Node.js 서버 예제(`server.js`)가 포함되어 있지 않습니다. `startSocketClient()`를 테스트하려면 `127.0.0.1:50313`에서 TCP 서버를 별도로 실행해야 합니다.
- 수신 파서는 TCP 스트림을 완전한 메시지 단위로 재조립하지 않습니다. 현재는 한 번의 `recv()` 호출에 완전한 `name:value1,value2` 문자열이 들어온다고 가정합니다.
- `PacketProcessor::parse()`는 `:`가 없는 데이터에 대한 방어 처리가 없습니다.
- `SocketClient::SendData()`는 전송 문자열에 `\n`을 추가하지만, 반환값 비교는 원본 문자열 길이와 비교하고 있어 성공 판정이 실제 전송 길이와 맞지 않습니다.
- `getMessage()`의 문자열 반환은 일부 경로에서 단순 byte-to-wide 변환을 사용하므로, 비ASCII 수신 데이터는 깨질 수 있습니다.
- `getMessage()`에는 테스트용 `JS_ReportError("TEST::테스트")` 호출이 남아 있어 정상 호출 중에도 에러 리포트를 발생시킬 수 있습니다.

