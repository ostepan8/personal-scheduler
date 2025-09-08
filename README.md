# Personal Scheduler

A full-stack personal scheduling application with a C++ backend API server and React frontend. Features include recurring events, calendar views, and Google Calendar integration.

## 🚀 Features

### Backend
- **RESTful API** with authentication and rate limiting
- **Recurring Events** with flexible patterns (daily, weekly, monthly, yearly)
- **SQLite Database** with automatic schema evolution
- **Google Calendar Integration** via Python service
- **Task Scheduling** with notifications and custom actions
- **Event Loop** for background task processing

### Frontend
- **Modern React UI** with TypeScript and Tailwind CSS
- **Multiple Calendar Views** (Day, Week, Month)
- **Interactive Event Management** with recurring event support
- **Real-time Dashboard** with today's events and statistics
- **12-hour Time Display** with proper formatting
- **Responsive Design** optimized for desktop and mobile

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   React Frontend │────│   C++ API Server  │────│  SQLite Database │
│   (Port 3004)    │    │   (Port 8080)     │    │   (events.db)    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │
                       ┌──────────────────┐
                       │ Google Calendar  │
                       │  (Python Service) │
                       └──────────────────┘
```

## 📦 Installation

### Prerequisites
- **C++17** compiler (g++ or clang++)
- **Node.js 16+** and npm
- **SQLite3** development libraries
- **Python 3.7+** (for Google Calendar integration)

### Backend Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ostepan8/personal-scheduler.git
   cd personal-scheduler
   ```

2. **Configure environment:**
   ```bash
   cp .env.example .env
   ```
   
   Edit `.env` with your settings:
   ```env
   API_KEY=your-secret-api-key
   ADMIN_API_KEY=your-admin-key
   HOST=127.0.0.1
   PORT=8080
   CORS_ORIGIN=http://localhost:3004
   RATE_LIMIT=100
   RATE_WINDOW=60
   ```

3. **Build and run the API server:**
   ```bash
   make api
   ./api_server
   ```

### Frontend Setup

1. **Install dependencies:**
   ```bash
   cd frontend
   npm install
   ```

2. **Configure environment:**
   ```bash
   cp .env.example .env.local
   ```
   
   Edit `.env.local`:
   ```env
   REACT_APP_API_URL=http://localhost:8080
   REACT_APP_API_KEY=your-secret-api-key
   ```

3. **Start the development server:**
   ```bash
   PORT=3004 npm start
   ```

4. **Open your browser:**
   Navigate to `http://localhost:3004`

## 🔧 Development

### Running Tests
```bash
make test
# or
./run_all_tests.sh
```

### Building for Production
```bash
# Backend
make api

# Frontend
cd frontend
npm run build
```

### Project Structure
```
scheduler/
├── api/                 # API routes and server
├── calendar/            # Google Calendar integration
├── database/            # SQLite database layer
├── model/               # Event and recurrence models
├── scheduler/           # Event loop and task scheduling
├── frontend/            # React application
│   ├── src/
│   │   ├── components/  # React components
│   │   ├── services/    # API client
│   │   └── types/       # TypeScript interfaces
│   └── public/          # Static assets
├── tests/               # Unit tests
└── Makefile            # Build configuration
```

## 🛠️ API Usage

### Authentication
All API requests require the `Authorization` header:
```bash
curl -H 'Authorization: your-api-key' http://localhost:8080/events
```

### Key Endpoints
- `GET /events` - List events
- `POST /events` - Create event
- `GET /events?expanded=true` - List with recurring expansions
- `GET /recurring` - List recurring event templates
- `POST /recurring` - Create recurring event
- `GET /stats/events/{start}/{end}` - Get event statistics

### Time Format
- **Input/Output:** Local time (`YYYY-MM-DD HH:MM`)
- **Internal Storage:** UTC timestamps
- **Frontend Display:** 12-hour format with AM/PM

## 🗓️ Calendar Integration

### Google Calendar Setup
1. Create a Google Cloud Project
2. Enable Calendar API
3. Create service account credentials
4. Download `credentials.json` to `calendar_integration/`
5. Share your calendar with the service account email

The integration automatically syncs events bidirectionally between your personal scheduler and Google Calendar.

## 🔒 Security Features

- **API Key Authentication** for all endpoints
- **Admin Key Protection** for destructive operations
- **Rate Limiting** to prevent abuse
- **CORS Configuration** for secure frontend access
- **Input Sanitization** for all user data

## 📊 Database Schema

Events are stored in SQLite with automatic schema evolution:

```sql
CREATE TABLE events (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    description TEXT,
    time INTEGER NOT NULL,  -- Unix timestamp (UTC)
    duration INTEGER NOT NULL,  -- Duration in seconds
    category TEXT,
    recurring INTEGER DEFAULT 0,
    -- ... additional fields for integrations
);
```

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **httplib** for C++ HTTP server functionality
- **nlohmann/json** for JSON parsing
- **React** and **Tailwind CSS** for the modern frontend
- **date-fns** for date manipulation utilities