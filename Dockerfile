FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV NORM_VER 3.3.53

WORKDIR /home/root
VOLUME /home/root

RUN printf "Installing Ubuntu and 42 stuff\n"

# Update and install required packages, including netcat-openbsd
RUN apt-get update && apt-get install -y \
	lsb-release valgrind clang wget nano python3 \
	python3-pip build-essential cmake git curl python3-venv \
	netcat-openbsd zsh \
	&& chsh -s $(which zsh)

# Install Oh My Zsh
RUN sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" || true


CMD ["zsh"]

