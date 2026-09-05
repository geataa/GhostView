@echo off
echo ===================================================================
echo   GhostView Windows Varsayilan Resim Goruntuleyici Ayarlayici
echo ===================================================================

echo 1. Kayit defteri ayarlari uygulaniyor...
reg import "%~dp0register_as_default.reg"

echo.
echo 2. Windows Varsayilan Uygulamalar sayfasi aciliyor...
start ms-settings:defaultapps

echo.
echo ===================================================================
echo   ISLEM TAMAMLANDI!
echo.
echo   Acilan Windows Ayarlari ekraninda:
echo   - Listeden "GhostView"i bulun veya arama cubuguna "GhostView" yazin.
echo   - "Varsayilan Yap" (Set Default) butonuna tiklayin!
echo.
echo   Ayrica herhangi bir resme sag tiklayip:
echo   "Birlikte Ac" -> "Baska bir uygulama sec" -> "GhostView"
echo   secip "Her Zaman" kutusunu isaretleyebilirsiniz.
echo ===================================================================
pause
