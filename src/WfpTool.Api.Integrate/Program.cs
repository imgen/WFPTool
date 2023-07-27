using System;

namespace WfpTool.Api.Integrate
{
    class Program
    {
        static void Main()
        {
            var handle = Win32.AddAllowTunnelFilter("Wi-Fi");
            if (handle <= 0)
            {
                var errorCode = Win32.GetLastFilterApiError();
                Console.WriteLine("Oops, AddAllowTunnelFilter failed with error code: " + errorCode);
            }
            else
            {
                var success = Win32.RemoveFilter(handle);
                if (success == 0)
                {
                    var errorCode = Win32.GetLastFilterApiError();
                    Console.WriteLine("Oops, unable to remove block all filter with error code: " + errorCode);
                }
            }

            var success2 = Win32.RemoveAllFilters();
            if (success2 <= 0)
            {
                var errorCode = Win32.GetLastFilterApiError();
                Console.WriteLine("Oops, unable to remove all filters with error code: " + errorCode);
            }
        }
    }
}
