# [VENOMICA CORE] The Zero-Trust Sandbox Environment (AGI Enabled)

FROM ubuntu:24.04

# 1. Install required tools INCLUDING the compiler (g++-14) so MutationRunner can dynamically compile
RUN apt-get update && apt-get install -y gcc-14 g++-14 cmake make python3 jq curl git bash && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100 && \
    rm -rf /var/lib/apt/lists/*

# 2. Create the unprivileged AGI user
RUN useradd -m -s /usr/sbin/nologin swayam_agent

# 3. Forge the Core Binary during build
WORKDIR /build_space
COPY . .
RUN mkdir -p build && cd build && cmake .. && make swayam_core

# 4. Vault the Binary & Cleanup Source Code (Zero-Trust Enforcement)
RUN mkdir -p /opt/swayam/bin && \
    find build -type f -name "swayam_core" -executable -exec mv {} /opt/swayam/bin/swayam_core \; && \
    chmod 755 /opt/swayam/bin/swayam_core && \
    rm -rf /build_space

# 5. Provision the Runtime Matrix Boundaries
RUN mkdir -p /home/swayam_agent/workspace/src/generated && \
    mkdir -p /home/swayam_agent/workspace/meta && \
    mkdir -p /home/swayam_agent/workspace/quarantine && \
    chown -R swayam_agent:swayam_agent /home/swayam_agent/workspace

USER swayam_agent
WORKDIR /home/swayam_agent/workspace

# THE ULTIMATE LOCK: Booting the true C++ core engine directly
ENTRYPOINT ["/opt/swayam/bin/swayam_core"]
