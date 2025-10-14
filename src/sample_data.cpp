#include "sample_data.h"

// Sample JSON calendar events in Home Assistant API format (for testing)
const char* sampleEventsJson = R"json({
  "entity_id": "sensor.esp32_calendar_data",
  "state": "23",
  "attributes": {
    "events": [
      {
        "title": "House cleaning service",
        "start": "2025-01-08T09:30:00+01:00",
        "end": "2025-01-08T11:00:00+01:00",
        "calendar": "family"
      },
      {
        "title": "School parent meeting",
        "start": "2025-01-08T14:45:00+01:00",
        "end": "2025-01-08T15:45:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Book parent teacher meeting",
        "start": "2025-01-08T19:00:00+01:00",
        "end": "2025-01-08T19:30:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Child flu vaccine at school",
        "start": "2025-01-09",
        "end": "2025-01-09",
        "calendar": "family"
      },
      {
        "title": "Swimming lesson",
        "start": "2025-01-10T15:45:00+01:00",
        "end": "2025-01-10T16:15:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Parent association meeting",
        "start": "2025-01-10T18:00:00+01:00",
        "end": "2025-01-10T19:00:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Buy event tickets",
        "start": "2025-01-13T19:00:00+01:00",
        "end": "2025-01-13T20:00:00+01:00",
        "calendar": "family"
      },
      {
        "title": "Reception Parents Reading and Phonics Meeting",
        "start": "2025-01-08T14:45:00+01:00",
        "end": "2025-01-08T23:59:00+01:00",
        "calendar": "school"
      },
      {
        "title": "Rescheduled: Year 2 Class Meet the Teacher Session",
        "start": "2025-01-08T15:15:00+01:00",
        "end": "2025-01-08T16:00:00+01:00",
        "calendar": "school"
      },
      {
        "title": "IMPORTANT: Reception &amp; Year 6 Flu Immunisation",
        "start": "2025-01-09",
        "end": "2025-01-09",
        "calendar": "school"
      },
      {
        "title": "Special Lunch Menu Day",
        "start": "2025-01-14",
        "end": "2025-01-14",
        "calendar": "school"
      },
      {
        "title": "Year 6 School Trip Meeting",
        "start": "2025-01-15T18:00:00+01:00",
        "end": "2025-01-15T23:59:00+01:00",
        "calendar": "school"
      },
      {
        "title": "Harvest Festival Competition",
        "start": "2025-01-16",
        "end": "2025-01-16",
        "calendar": "school"
      },
      {
        "title": "Year 2 Festival Performance",
        "start": "2025-01-16T09:15:00+01:00",
        "end": "2025-01-16T23:59:00+01:00",
        "calendar": "school"
      },
      {
        "title": "Year 4 Festival Performance",
        "start": "2025-01-16T10:15:00+01:00",
        "end": "2025-01-16T23:59:00+01:00",
        "calendar": "school"
      },
      {
        "title": "Wear Red Day 2025: Charity Event",
        "start": "2025-01-17",
        "end": "2025-01-17",
        "calendar": "school"
      },
      {
        "title": "School Disco; Year 1",
        "start": "2025-01-17T15:15:00+01:00",
        "end": "2025-01-17T16:15:00+01:00",
        "calendar": "school"
      },
      {
        "title": "School Disco; Years 2, 3",
        "start": "2025-01-17T16:45:00+01:00",
        "end": "2025-01-17T18:00:00+01:00",
        "calendar": "school"
      },
      {
        "title": "School Disco; Years 4, 5, 6",
        "start": "2025-01-17T18:30:00+01:00",
        "end": "2025-01-17T19:45:00+01:00",
        "calendar": "school"
      },
      {
        "title": "Coffee Meeting",
        "start": "2025-01-08T08:00:00+01:00",
        "end": "2025-01-08T08:30:00+01:00",
        "calendar": "work"
      },
      {
        "title": "Status Update",
        "start": "2025-01-08T10:30:00+01:00",
        "end": "2025-01-08T11:00:00+01:00",
        "calendar": "work"
      },
      {
        "title": "Client Presentation",
        "start": "2025-01-08T11:30:00+01:00",
        "end": "2025-01-08T12:30:00+01:00",
        "calendar": "work"
      },
      {
        "title": "Email Catchup",
        "start": "2025-01-08T14:00:00+01:00",
        "end": "2025-01-08T14:30:00+01:00",
        "calendar": "work"
      }
    ],
    "current_date": "2025-01-09",
    "current_day": "Thursday",
    "current_time": "20:48:00",
    "week_start": "2025-01-06",
    "period": "2025-01-06 to 2025-01-19",
    "friendly_name": "ESP32 Calendar Data"
  },
  "last_changed": "2025-01-09T17:07:00.150903+00:00",
  "last_reported": "2025-01-09T19:48:00.151604+00:00",
  "last_updated": "2025-01-09T19:48:00.151604+00:00",
  "context": {
    "id": "01K75887GQY6HK30J9FMJ1ESSS",
    "parent_id": null,
    "user_id": null
  }
})json";
