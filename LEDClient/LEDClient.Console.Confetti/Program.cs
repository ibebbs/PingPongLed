using Flurl;
using Flurl.Http;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace LEDClient.Console.Confetti
{
    class Program
    {
        private const string Server = "http://192.168.1.71/";
        private const string LedsPath = "leds";

        private const int LedCount = 50;

        private static readonly Random Random = new Random(DateTime.Now.Millisecond);
        private static readonly Spectrum.Color.RGB[] Leds = Enumerable.Range(0, 50).Select(_ => new Spectrum.Color.RGB(0, 0, 0)).ToArray();

        private static string FromLeds()
        {
            return string.Join("-", Leds.Select(color => $"{color.R.ToString("X2")}{color.G.ToString("X2")}{color.B.ToString("X2")}"));
        }

        private static void FadeAll(int by)
        {
            double fade = (255.0 - by) / 255.0;

            for (int i = 0; i < 50; i++)
            {
                Leds[i] = new Spectrum.Color.RGB(Convert.ToByte(Leds[i].R * fade), Convert.ToByte(Leds[i].G * fade), Convert.ToByte(Leds[i].B * fade));
            }
        }

        private static void Show()
        {
            var result = Server.AppendPathSegment(LedsPath).SetQueryParams(new { value = FromLeds() }).PostStringAsync(string.Empty).Result;
        }

        private static byte _hue = 0;

        static void Main(string[] args)
        {
            while (true)
            {
                _hue++;

                // random colored speckles that blink in and fade smoothly
                FadeAll(10);

                int pos = Random.Next(LedCount);
                Leds[pos] = new Spectrum.Color.HSV(_hue + Random.Next(16), 0.8, 1).ToRGB();

                Show();

                Thread.Sleep(TimeSpan.FromMilliseconds(10));
            }
        }
    }
}
