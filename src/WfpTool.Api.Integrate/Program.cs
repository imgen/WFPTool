using System;

namespace WfpTool.Api.Integrate
{
    class Program
    {
        static void Main()
        {
            var handle = Win32.AddAllowTunnelFilter("Nodapter");
            if (handle <= 0)
            {
                var errorCode = Win32.GetLastFilterApiError();
                Console.WriteLine("Oops, AddAllowTunnelFilter failed with error code: " + errorCode);
            }
            var success = Win32.RemoveFilter(handle);
            if (success == 0)
            {
                var errorCode = Win32.GetLastFilterApiError();
                Console.WriteLine("Oops, unable to remove block all filter with error code: " + errorCode);
            }

            success = Win32.RemoveAllFilters();
            if (success == 0)
            {
                Console.WriteLine("Oops, unable to remove all filters");
            }
        }
    }
}
