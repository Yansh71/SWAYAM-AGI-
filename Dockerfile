# [VENOMICA CORE] The Zero-Trust Sandbox Environment
FROM ubuntu:24.04

# Install only necessary toolchains for the agent
RUN apt-get update && apt-get install -y \
    g++ python3 git curl jq \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for extreme sandbox security
RUN useradd -m -s /bin/bash swayam_agent
USER swayam_agent
WORKDIR /home/swayam_agent/workspace

# Copy files with correct ownership
COPY --chown=swayam_agent:swayam_agent . .

# Default fallback command. The pipeline will override this during execution.
CMD ["echo", "[VENOMICA] Sandbox Environment Ready and Standing By."]
