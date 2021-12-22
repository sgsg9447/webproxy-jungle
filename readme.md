**탐험준비 - Week07  웹서버 만들기**

<aside>
💡 나만의 웹서버를 만들어보기! (프록시 서버까지)

</aside>

- 클라이언트의 request를 받고, response를 내어주는 웹서버를 만들어봅니다.
    
    ㄴ 웹서버는 어떤 기능들의 모음일까요?
    
- '컴퓨터 시스템' 교재의 11장을 보면서 차근 차근 만들어주세요.(기본 코드는 모두 있습니다!)
- 웹 서버를 완성했으면 프록시(proxy) 서버 과제에 도전합니다.
    - [http://csapp.cs.cmu.edu/3e/proxylab.pdf](http://csapp.cs.cmu.edu/3e/proxylab.pdf)
    - 출처: CMU (카네기멜론)
- [https://github.com/SWJungle/webproxy-jungle](https://github.com/SWJungle/webproxy-jungle) 의 내용대로 진행합니다.
    - 진행방법
        1. 책에 있는 코드를 기반으로, tiny 웹서버를 완성하기 (tiny/tiny.c, tiny/cgi-bin/adder.c 완성)
        2. AWS 혹은 container 사용시 외부로 포트 여는 것을 잊지 말기
        3. 숙제 문제 풀기 (11.6c, 7, 9, 10, 11)
        4. 프록시 과제 도전 (proxy.c 완성)
    - 참고내용
        - HTTP/1.0 표준: [RFC-1945](https://datatracker.ietf.org/doc/html/rfc1945)
        - Media (MIME) type
            - 표준: [RFC-2046](https://datatracker.ietf.org/doc/html/rfc2046)
            - 등록 현황: [https://www.iana.org/assignments/media-types/media-types.xhtml](https://www.iana.org/assignments/media-types/media-types.xhtml)
        - HTTP/1.1 표준: RFC-[7230](https://datatracker.ietf.org/doc/html/rfc7230) ~ [7240](https://datatracker.ietf.org/doc/html/rfc7240) ([RFC-2616](https://datatracker.ietf.org/doc/html/rfc2616)은 obsolete 됨)
        - HTTP/2 표준: [RFC-7540](https://datatracker.ietf.org/doc/html/rfc7540)
        - HTTP/3
            - 표준안(34차): [draft-ietf-quic-http-34](https://datatracker.ietf.org/doc/html/draft-ietf-quic-http-34)
            - 현재 구현 상황: [Wikipedia](https://en.wikipedia.org/wiki/HTTP/3#Implementations)