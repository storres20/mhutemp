# TEMPHU: An Open-Source IoT Solution for Reliable Real-Time Temperature and Humidity Monitoring in Healthcare Facilities

**TEMPHU** (Temperature and Humidity Monitoring Unit) is an open-source IoT-based system designed to provide real-time environmental monitoring for healthcare applications, particularly blood bank refrigerators.  
The project offers an affordable and scalable alternative to traditional commercial monitoring systems, ensuring full control over data and system customization without subscription fees.

---

## 📚 Project Structure

The TEMPHU system consists of two main components, maintained in separate repositories:

- **Backend Server** (Node.js + MongoDB):  
  [https://github.com/storres20/bio-data](https://github.com/storres20/bio-data)

- **Frontend Application** (Next.js):  
  [https://github.com/storres20/bio-data-nextjs](https://github.com/storres20/bio-data-nextjs)

**Live Web Dashboard:**  
[https://bio-data-nextjs.netlify.app/data](https://bio-data-nextjs.netlify.app/data)

---

## 🚀 Features

- Real-time temperature and humidity monitoring using ESP8266 microcontrollers.
- DS18B20 sensor for high-precision temperature measurements.
- DHT11 sensor for ambient temperature and humidity monitoring.
- Local OLED display for on-site visual feedback.
- WebSocket-based real-time data updates every 2 seconds.
- Historical data storage in MongoDB with date-based filtering.
- Responsive web dashboard for real-time and historical data visualisation.

---

## 🔧 Technologies Used

- **Microcontroller:** ESP8266 (Arduino C/C++)
- **Sensors:** DS18B20, DHT11
- **Backend:** Node.js, Express.js, MongoDB
- **Frontend:** Next.js (React.js framework)
- **Communication:** HTTP POST (data submission) and WebSocket (real-time updates)

---

## 🛠️ Installation Instructions

Please refer to the individual repositories for setup and deployment instructions:

- Backend setup guide: [https://github.com/storres20/bio-data](https://github.com/storres20/bio-data)
- Frontend setup guide: [https://github.com/storres20/bio-data-nextjs](https://github.com/storres20/bio-data-nextjs)

---

## ⚡ Quick Start

1. Clone the backend and frontend repositories separately.
2. Set up your MongoDB database and configure the environment variables.
3. Deploy the backend server and frontend application.
4. Connect your ESP8266 devices to the backend API endpoint.
5. Start monitoring real-time environmental conditions.

---

## 📜 License

This project is licensed under the [MIT License](https://github.com/storres20/temphu/blob/main/LICENSE.txt).

---

## 🤝 Contributions

Contributions, suggestions, and improvements are welcome!  
Feel free to fork the repositories, open issues, and submit pull requests.

---

## 📬 Contact

If you have any questions, please open an issue in the corresponding GitHub repository.
