# 베이스 이미지로 Python 3.10-slim 사용
FROM python:3.10-slim

# 패키지 설치 및 업데이트
RUN apt-get update && \
    apt-get install -y \
    gcc \
    make \
    curl \
    git \
    libcurl4-openssl-dev \
    libcjson-dev \
    && apt-get clean

# cJSON 라이브러리 클론 및 빌드
RUN git clone https://github.com/DaveGamble/cJSON.git /cJSON && \
    cd /cJSON && \
    make && \
    make install

# cJSON 헤더 파일 위치 확인
RUN find / -name "cJSON.h"

# 작업 디렉토리 설정
WORKDIR /app

# ui_1 폴더를 클론하여 /app 디렉토리에 복사 (이미 GitHub에서 클론할 필요 없음)
# GitHub에서 리포지토리를 클론하는 대신 ui_1 폴더가 이미 포함된 상태라면
# 아래 부분을 생략할 수 있습니다.

# /app/ui_1 디렉토리로 이동
WORKDIR /app/ui_1

# 현재 디렉토리에서 requirements.txt를 복사
COPY ui_1/requirements.txt ./  # ui_1 폴더 내의 requirements.txt 파일 복사

# 필요한 Python 패키지 설치
RUN pip install --no-cache-dir -r requirements.txt

# Flask 앱 실행
CMD ["python", "app.py"]  # app.py가 main 파일이라면 실행
