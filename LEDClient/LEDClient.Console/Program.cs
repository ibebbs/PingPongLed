using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Flurl;
using Flurl.Http;

namespace LEDClient.Console
{
    class Program
    {
        private const string Server = "http://192.168.1.103/";
        private const string Leds = "leds";

        private static readonly Random Random = new Random(DateTime.Now.Millisecond);

        static void Main(string[] args)
        {
            while (true)
            {
                var value = string.Join("-", Enumerable.Range(0, 50).Select(index => string.Join(string.Empty, Enumerable.Range(0, 3).Select(_ => Random.Next(255).ToString("X2")))));

                //System.Console.WriteLine($"Sending '{value}'");

                var result = Server.AppendPathSegment(Leds).SetQueryParams(new { value }).PostStringAsync(string.Empty).Result;

                //System.Console.WriteLine(result);
            }                
        }
    }
}
