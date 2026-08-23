# [VENOMICA CORE] The Zero-Trust Sandbox Environment

# ==========================================
# STAGE 1: THE FORGE (Secure Build Phase)
# ==========================================
FROM ubuntu:24.04 AS builder

# Install build dependencies (These will NEVER reach the final runtime)
RUN apt-get update && apt-get install -y gcc-14 g++-14 cmake make && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

WORKDIR /build_space
COPY . .

# Deterministic Compilation: 
# Build the project and explicitly extract ONLY the swayam_supervisor binary
RUN mkdir -p build && cd build && cmake .. && make swayam_supervisor && \
    find . -type f -name "swayam_supervisor" -executable -exec mv {} /swayam_supervisor_bin \;

# ==========================================
# STAGE 2: THE VAULT (Immutable Runtime)
# ==========================================
FROM ubuntu:24.04

# Install ONLY bare-metal runtime necessities (No GCC, No Compilers!)
RUN apt-get update && apt-get install -y python3 jq curl git bash && \
    rm -rf /var/lib/apt/lists/*

# Create unprivileged user with NO SHELL LOGIN (Prevents TTY terminal hijacking)
RUN useradd -m -s /usr/sbin/nologin swayam_agent

# Establish the Immutable Binary Path (Owned by ROOT, read-only for agent)
RUN mkdir -p /opt/swayam/bin
COPY --from=builder --chown=root:root /swayam_supervisor_bin /opt/swayam/bin/swayam_supervisor
RUN chmod 755 /opt/swayam/bin/swayam_supervisor

# Establish explicit workspace mounts for the GitHub CI/CD volumes
RUN mkdir -p /home/swayam_agent/workspace/src/generated && \
    mkdir -p /home/swayam_agent/workspace/meta && \
    chown -R swayam_agent:swayam_agent /home/swayam_agent/workspace

USER swayam_agent
WORKDIR /home/swayam_agent/workspace

# THE ULTIMATE LOCK: Hardcoded Entrypoint. 
# Cannot be bypassed or overridden by 'docker run bash'
ENTRYPOINT ["/opt/swayam/bin/swayam_supervisor"]
