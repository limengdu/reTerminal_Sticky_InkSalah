# Prayer calculation

InkSalah calculates prayer boundaries on the device. It does not download a city timetable. The result is determined by:

1. civil date;
2. latitude and longitude;
3. fixed UTC offset;
4. selected twilight profile;
5. Standard or Hanafi Asr shadow rule; and
6. optional per-time minute adjustments.

## Boundaries and windows

| Boundary | Meaning in InkSalah | Window that ends there |
| --- | --- | --- |
| Fajr | Dawn boundary selected by the calculation profile | Previous night's Isha |
| Sunrise | Sun center corrected to approximately −0.833° | Fajr |
| Dhuhr | Local solar noon | Between-prayer daytime period |
| Asr | Afternoon shadow boundary | Dhuhr |
| Maghrib | Sunset or a profile-specific Maghrib boundary | Asr |
| Isha | Evening twilight angle or a fixed interval after Maghrib | Maghrib |
| Qiyam | Start of the final third of the night | Displayed as an overlapping voluntary period |

Isha remains active until the next Fajr. When Qiyam display is enabled, Qiyam is shown as an additional night marker rather than replacing or shortening Isha.

## Calculation profiles

The short names used on the device match widely recognized names from the [AlAdhan calculation method catalogue](https://api.aladhan.com/v1/methods). Angles below are solar depression angles.

| Device name | Profile | Fajr | Isha | Additional rule |
| --- | --- | ---: | ---: | --- |
| `JAFARI` | Shia Ithna-Ashari, Leva Institute, Qum | 16° | 14° | Maghrib 4° |
| `KARACHI` | University of Islamic Sciences, Karachi | 18° | 18° | — |
| `ISNA` | Islamic Society of North America | 15° | 15° | — |
| `MWL` | Muslim World League | 18° | 17° | — |
| `UMM AL-QURA` | Umm Al-Qura University, Makkah | 18.5° | — | Isha 90 min after Maghrib |
| `EGYPT` | Egyptian General Authority of Survey | 19.5° | 17.5° | — |
| `TEHRAN` | Institute of Geophysics, University of Tehran | 17.7° | 14° | Maghrib 4.5° |
| `GULF` | Gulf Region | 19.5° | — | Isha 90 min after Maghrib |
| `KUWAIT` | Kuwait | 18° | 17.5° | — |
| `QATAR` | Qatar | 18° | — | Isha 90 min after Maghrib |
| `MUIS` | Majlis Ugama Islam Singapura | 20° | 18° | — |
| `UOIF` | Union des Organisations Islamiques de France | 12° | 12° | — |
| `DİB` | Diyanet İşleri Başkanlığı, Turkey | 18° | 17° | — |
| `RUSSIA` | Spiritual Administration of Muslims of Russia | 16° | 15° | — |
| `MOONSIGHTING` | Moonsighting Committee Worldwide | seasonal | seasonal | Season- and latitude-aware safe twilight |
| `DUBAI` | Dubai | 18.2° | 18.2° | — |
| `JAKIM` | Jabatan Kemajuan Islam Malaysia | 20° | 18° | — |
| `TUNISIA` | Tunisian Ministry of Religious Affairs | 18° | 18° | — |
| `ALGERIA` | Algerian Ministry of Religious Affairs and Wakfs | 18° | 17° | — |
| `KEMENAG` | Kementerian Agama Republik Indonesia | 20° | 18° | — |
| `MOROCCO` | Morocco | 19° | 17° | — |
| `PORTUGAL` | Comunidade Islâmica de Lisboa | 18° | — | Isha 77 min after Maghrib; Maghrib +3 min |
| `JORDAN` | Jordan | 18° | 18° | Maghrib +5 min |
| `CUSTOM ANGLES` | User-defined angles | 10–25° | 10–25° | — |
| `CUSTOM INTERVAL` | User-defined Fajr and Isha interval | 10–25° | — | Isha 1–180 min after Maghrib |

The default profile is `MWL`. China uses the project default of `MWL + HANAFI`; automatic country recommendations may select another locally common profile.

## Asr rule

InkSalah supports two independent Asr choices:

- `STANDARD`: shadow factor 1, used by the Shafi, Maliki, and Hanbali schools.
- `HANAFI`: shadow factor 2, producing a later Asr start.

Changing the Asr school affects only Asr.

## Qiyam

Qiyam is calculated as the beginning of the last third of the night:

```text
night length = next Fajr - current Maghrib
Qiyam = current Maghrib + 2/3 × night length
```

The calculation deliberately uses the next day's Fajr, so the firmware calculates adjacent days together.

## High latitudes

For angle-based profiles, Fajr and Isha are limited to an angle-based fraction of the available night when the requested twilight angle does not occur or would produce an extreme result. The `MOONSIGHTING` profile uses season- and latitude-aware morning and evening safe intervals, with a one-seventh night rule at very high latitudes.

These rules provide a stable mathematical result; local authorities may publish a different high-latitude convention. The independent minute adjustments can align the display with a trusted local timetable.

## Precision and known limits

- Results are rounded to the nearest minute.
- Dhuhr currently uses local solar noon without an automatic post-zawal delay.
- The time zone is a fixed UTC offset, not an IANA time-zone database. Daylight-saving changes require the offset to be reviewed.
- IP-based location is approximate and should be confirmed on the device.
- Atmospheric and local-horizon conditions can shift observed sunrise and sunset.

For congregational prayer, use the local mosque's timetable and iqamah schedule as the practical reference.
