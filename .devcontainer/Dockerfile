FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && apt-get install -y sudo

# Create a non-root user
RUN useradd -m developer && echo "developer:developer" | chpasswd && usermod -aG sudo developer

# Set the default user
USER developer

# Set working directory
WORKDIR /workspace
