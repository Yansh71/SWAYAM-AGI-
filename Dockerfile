# [VENOMICA CORE] The Zero-Trust Sandbox Environment
FROM ubuntu:24.04

# Install only necessary toolchains pinned strictly to GCC-14
RUN apt-get update && apt-get install -y gcc-14 g++-14 python3 git curl jq && \
    rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

# Create a non-root user for extreme sandbox security
RUN useradd -m -s /bin/bash swayam_agent
USER swayam_agent
WORKDIR /home/swayam_agent/workspace

# Copy files with correct ownership
COPY --chown=swayam_agent:swayam_agent . .

# CRITICAL: build & execute the candidate mutation via testing script
CMD ["bash", "scripts/testing/run_sandbox.sh"]
