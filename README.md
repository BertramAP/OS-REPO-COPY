## Testing Rig & Docker Setup

This test rig emulates the VM environment with the following constrains: 
- 4 CPU cores
- 512 MB RAM
## Usage Guide

Here are the standard commands for operating the test rig:

### Server Management

Start your server in the background:
```bash
docker compose up -d server
```

Rebuild your server and restart it (useful after making code changes):
```bash
docker compose up -d --build server
```

Watch your server's output/logs:
```bash
docker compose logs server
```

Start the reference server instead:
```bash
docker compose up -d reference
```

Stop everything immediately or gracefully:
```bash
docker compose kill  # Immediate shutdown
docker compose down  # Graceful shutdown and removal
```

### Running Tests

To run the clients against your server (which is the default target), use the following commands. The `--rm` flag ensures the client container cleans itself up after the run is complete.

**1. Debug / Single Request:**
```bash
docker compose run --rm single
```

**2. Smoke Test (20 easy requests):**
```bash
docker compose run --rm smoke
```

**3. Graded Milestone Run (100 hard requests):**
```bash
docker compose run --rm milestone
```

---

## Targeting the Reference Server

By default, running a client container will send traffic to your `server` service. If you want to benchmark or test the `reference` server instead, you can override the `$TARGET` environment variable.

**On Bash (Linux / macOS):**
```bash
TARGET=reference docker compose run --rm milestone
```

**On PowerShell (Windows):**
```powershell
$env:TARGET="reference"; docker compose run --rm milestone
```


## About README
Most of the overview is made by generative AI, with the provided context of the docker-compose.yml file. The README purpose is to give a quick overview how to use the different docker commands to run test of the server code.
