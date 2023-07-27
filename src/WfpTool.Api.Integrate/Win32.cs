using System.Runtime.InteropServices;

namespace WfpTool.Api.Integrate
{
    public static class Win32
    {
        const string FilterApiDll = "WfpTool.Api.dll";

        [DllImport(FilterApiDll)]
        public static extern uint GetLastFilterApiError();

        [DllImport(FilterApiDll)]
        public static extern uint AddBlockAllFilter();

        [DllImport(FilterApiDll)]
        public static extern uint AddAllowTunnelFilter([MarshalAs(UnmanagedType.LPWStr)]string adapterNameOrId);

        [DllImport(FilterApiDll)]
        public static extern uint AddAllowDnsFilter();

        [DllImport(FilterApiDll)]
        public static extern uint AddAllowSpecificFilter([MarshalAs(UnmanagedType.LPStr)]string ip, int protocol, int port);

        [DllImport(FilterApiDll)]
        public static extern int RemoveFilter(uint filterHandle);

        [DllImport(FilterApiDll)]
        public static extern int RemoveAllFilters();
    }
}