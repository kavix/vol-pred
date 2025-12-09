# Next.js Frontend Integration Prompt

Use this prompt to generate the frontend components for your existing Next.js application.

---

**Context:**
I have a backend API running at `http://localhost:3000` (or configured via `NEXT_PUBLIC_API_URL`) that provides energy monitoring data from an ESP32 device and ML predictions.

**API Endpoints:**
1.  **Real-time Data**: `GET /current`
    *   Returns: `{ volt: number, amps: number, watt: number, temperature: number, humidity: number, time: string }`
2.  **AI Prediction**: `GET /predict`
    *   Returns: `{ success: true, predictions: { watt: number, temperature: number, humidity: number }, units: object }`
3.  **Historical Data**: `GET /history?start=ISO_DATE&end=ISO_DATE&limit=100`
    *   Returns: Array of sensor objects.

**Task:**
Please implement the following components in my Next.js project using **Tailwind CSS** for styling and **SWR** for data fetching.

### 1. `components/EnergyDashboard.tsx`
Create a dashboard component that displays real-time sensor data and the latest AI prediction.
*   **Real-time Section**: Fetch `/current` every 2 seconds. Display Voltage, Current, Power, and Temperature in nice cards.
*   **Prediction Section**: Fetch `/predict` every 10 seconds. Display the predicted Power and Temperature for the next cycle.
*   Handle loading and error states gracefully.

### 2. `pages/history.tsx` (or `app/history/page.tsx`)
Create a page to view historical logs.
*   Include **Date Pickers** for Start Date and End Date.
*   Include a **"Filter" button** to trigger the API call to `/history`.
*   Display the results in a clean **Table** showing Time, Voltage, Current, and Power.
*   Show a loading indicator while fetching.

**Technical Requirements:**
*   Use `swr` for the dashboard auto-refreshing.
*   Use standard `fetch` for the history page (on-demand).
*   Ensure the UI is responsive (mobile-friendly).
