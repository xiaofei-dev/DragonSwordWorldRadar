namespace DragonSwordWorldRadar.Installer
{
    public interface IDataProvider
    {
        string Id { get; }
        DataSetResult Generate(InstallationContext context);
    }
}
