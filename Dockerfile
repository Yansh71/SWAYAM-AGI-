# [VENOMICA CORE] The Zero-Trust Sandbox Environment

# ==========================================
# STAGE 1: THE FORGE (Secure Build Phase)
# ==========================================
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y gcc-14 g++-14 cmake make && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

WORKDIR /build_space
COPY . .

# [TARGET FIXED] Explicitly compile and extract swayam_core
RUN mkdir -p build && cd build && cmake .. && make swayam_core && \
    find . -type f -name "swayam_core" -executable -exec mv {} /swayam_core_bin \;

# ==========================================
# STAGE 2: THE VAULT (Immutable Runtime)
# ==========================================
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y python3 jq curl git bash && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /usr/sbin/nologin swayam_agent

RUN mkdir -p /opt/swayam/bin
# [TARGET FIXED] Vaulting the core engine
COPY --from=builder --chown=root:root /swayam_core_bin /opt/swayam/bin/swayam_core
RUN chmod 755 /opt/swayam/bin/swayam_core

RUN mkdir -p /home/swayam_agent/workspace/src/generated && \
    mkdir -p /home/swayam_agent/workspace/meta && \
    chown -R swayam_agent:swayam_agent /home/swayam_agent/workspace

USER swayam_agent
WORKDIR /home/swayam_agent/workspace

# THE ULTIMATE LOCK: Booting the true C++ core engine directly
ENTRYPOINT ["/opt/swayam/bin/swayam_core"]
