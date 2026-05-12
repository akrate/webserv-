# WebServer

A lightweight, high-performance web server built for simplicity and speed.

---

## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [API Reference](#api-reference)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- Static file serving with MIME type detection
- HTTP/1.1 and HTTP/2 support
- SSL/TLS support via configurable certificates
- Request routing with dynamic URL parameters
- Middleware support (logging, authentication, compression)
- Configurable request rate limiting
- CORS headers management
- Access and error logging

---

## Prerequisites

- **Node.js** >= 18.x (or your runtime of choice)
- **npm** >= 9.x
- OpenSSL (for HTTPS support)

---

## Installation

```bash
# Clone the repository
git clone https://github.com/your-username/webserver.git
cd webserver

# Install dependencies
npm install
```

---

## Configuration

Create a `.env` file in the root directory or edit `config.json`:

```env
HOST=0.0.0.0
PORT=8080
SSL_ENABLED=false
SSL_CERT=./certs/cert.pem
SSL_KEY=./certs/key.pem
LOG_LEVEL=info
ROOT_DIR=./public
```

| Key          | Default       | Description                          |
|--------------|---------------|--------------------------------------|
| `HOST`       | `0.0.0.0`     | IP address to bind the server        |
| `PORT`       | `8080`        | Port the server listens on           |
| `SSL_ENABLED`| `false`       | Enable HTTPS                         |
| `SSL_CERT`   | —             | Path to the SSL certificate file     |
| `SSL_KEY`    | —             | Path to the SSL private key file     |
| `LOG_LEVEL`  | `info`        | Logging verbosity (`debug`, `info`, `error`) |
| `ROOT_DIR`   | `./public`    | Root directory for static files      |

---

## Usage

### Start the server

```bash
# Development (with auto-reload)
npm run dev

# Production
npm start
```

### Stop the server

Press `Ctrl + C` in the terminal, or send a `SIGTERM` signal:

```bash
kill -SIGTERM <pid>
```

### Generate a self-signed SSL certificate (development only)

```bash
openssl req -x509 -newkey rsa:4096 -keyout certs/key.pem -out certs/cert.pem -days 365 -nodes
```

---

## API Reference

### Health Check

```
GET /health
```

**Response:**

```json
{
  "status": "ok",
  "uptime": 3600
}
```

### Static Files

Files placed in the `ROOT_DIR` directory are served automatically:

```
GET /index.html     → ./public/index.html
GET /assets/app.js  → ./public/assets/app.js
```

### Custom Routes

Define routes in `routes/index.js`:

```js
router.get('/hello', (req, res) => {
  res.send('Hello, World!');
});

router.post('/data', (req, res) => {
  const body = req.body;
  res.json({ received: body });
});
```

---

## Project Structure

```
webserver/
├── certs/              # SSL certificates (not committed)
├── config/
│   └── config.json     # Server configuration
├── public/             # Static files root
├── routes/
│   └── index.js        # Route definitions
├── middleware/
│   ├── logger.js       # Request logging
│   ├── auth.js         # Authentication middleware
│   └── cors.js         # CORS headers
├── logs/
│   ├── access.log      # HTTP access log
│   └── error.log       # Error log
├── server.js           # Entry point
├── .env                # Environment variables (not committed)
├── package.json
└── README.md
```

---

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add my feature"`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a pull request

Please follow the existing code style and include tests for new features.

---

## License

This project is licensed under the [MIT License](LICENSE).
